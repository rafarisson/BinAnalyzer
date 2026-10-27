#include <QCoreApplication>

#include "bina/parse/BinaryParser.h"
#include "bina/format/ValueDecoder.h"
#include "bina/format/FieldFormat.h"

// bina::schema
#include "bina/schema/DataType.h"
#include "bina/schema/Field.h"
#include "bina/schema/LayoutCalculator.h"
#include "bina/schema/Builtins.h"
#include "bina/schema/TypeRegistry.h"

namespace bina::parse {

namespace {

qint64 alignUp(qint64 value, int align)
{
    return (value + align - 1) / align * align;
}

// Bytes do intervalo [offset, offset + size) que existem no arquivo.
QByteArray sliceOf(QByteArrayView data, qint64 offset, qint64 size)
{
    if (offset >= data.size())
        return {};

    return data.sliced(offset, qMin(size, data.size() - offset)).toByteArray();
}

}

BinaryParser::BinaryParser(const schema::TypeRegistry *types, QSysInfo::Endian endian, int maxAlign)
    : m_types{types}
    , m_endian{endian}
    , m_calculator{types, maxAlign}
{
}

bool BinaryParser::fail(const QString &message)
{
    m_error = message;
    return false;
}

schema::OperationResult BinaryParser::parse(const schema::Field *root, QByteArrayView data, ParseResult &out, qint64 maxRecords)
{
    out = ParseResult();

    if (!root || !m_types)
        return schema::OperationResult::failure(QCoreApplication::translate("bina", "Não há estrutura para analisar"));

    const auto layout = m_calculator.structLayout(root);
    if (layout.size <= 0)
        return schema::OperationResult::failure(QCoreApplication::translate("bina", "A estrutura está vazia ou usa tipos desconhecidos"));

    ParseResult result;
    result.structure = root;
    result.recordSize = layout.size;
    result.dataSize = data.size();

    if (const schema::OperationResult more = parseMore(result, data, maxRecords); !more)
        return more;

    out = std::move(result);

    return schema::OperationResult::success();
}

schema::OperationResult BinaryParser::parseMore(ParseResult &result, QByteArrayView data, qint64 count)
{
    m_error.clear();

    if (!result.structure || !m_types || result.recordSize <= 0)
        return schema::OperationResult::failure(QCoreApplication::translate("bina", "Não há estrutura para analisar"));

    if (data.size() != result.dataSize)
        return schema::OperationResult::failure(QCoreApplication::translate("bina", "Os dados mudaram desde a análise"));

    const schema::Field *root = result.structure;
    const qint64 first = result.parsedCount();
    const qint64 last = count < 0 ? result.recordCount() : qMin(result.recordCount(), first + count);

    for (qint64 i = first; i < last; ++i) {
        ParsedNode record;
        record.field = root;
        record.name = QStringLiteral("record[%1]").arg(i);
        record.instance = i;
        record.target = format::targetFor(*root, *m_types);

        if (!parseStructInstance(root, i * result.recordSize, result.recordSize, data, record))
            return schema::OperationResult::failure(m_error);

        result.records.push_back(std::move(record));
    }

    return schema::OperationResult::success();
}

// Campos de 'definition' (sem auto_pad) a partir de 'base'. O padding entre campos é implícito:
// cada campo começa no próximo offset alinhado, como no cálculo de layout do schema.
bool BinaryParser::parseFields(const schema::Field *definition, qint64 base, QByteArrayView data, QList<ParsedNode> &out)
{
    qint64 relative = 0;

    for (int i = 0; i < definition->childCount(); ++i) {
        const schema::Field *field = definition->child(i);
        if (field->isAutoPad())
            continue;

        const int align = m_calculator.layoutOf(field).align;
        relative = alignUp(relative, align);

        ParsedNode node;
        if (!parseField(field, base + relative, data, node))
            return false;

        relative += node.size;
        out.append(std::move(node));
    }

    return true;
}

bool BinaryParser::parseField(const schema::Field *field, qint64 offset, QByteArrayView data, ParsedNode &out)
{
    const int total = m_calculator.sizeOf(field);
    if (total <= 0)
        return fail(QCoreApplication::translate("bina", "O campo '%1' tem tipo ou tamanho inválido").arg(field->name()));

    out.field = field;
    out.name = field->name();
    out.offset = offset;
    out.size = total;
    out.truncated = offset + total > data.size();
    out.target = format::targetFor(*field, *m_types);

    const schema::DataType *type = m_types->find(field->typeId());
    const schema::Field *definition = nullptr;

    if (field->isStruct())
        definition = field;
    else if (type)
        definition = type->definition();

    if (!definition) {
        // Escalar, texto ou buffer: só é decodificado se estiver completo.
        if (!out.truncated)
            out.value = format::decodeValue(type->representation(), type->size(), field->count(),
                                            data.sliced(offset, total), m_endian);
        return true;
    }

    const int stride = m_calculator.layoutOf(field).size;

    if (field->count() == 1)
        return parseStructInstance(definition, offset, stride, data, out);

    // Array de structs: uma instância por elemento, só até onde o arquivo alcança.
    QList<format::Value> values;

    for (int i = 0; i < field->count() && offset + qint64(i) * stride < data.size(); ++i) {
        ParsedNode instance;
        if (!parseStructInstance(definition, offset + qint64(i) * stride, stride, data, instance))
            return false;

        instance.field = field;
        instance.name = QStringLiteral("%1[%2]").arg(field->name()).arg(i);
        instance.instance = i;
        instance.target = out.target;
        instance.target.count = 1;

        values.append(instance.value);
        out.children.append(std::move(instance));
    }

    if (!out.truncated)
        out.value = format::Value::makeArray(values, sliceOf(data, offset, total));

    return true;
}

// Uma instância de struct: os filhos de 'definition' começando em 'offset'.
// O chamador define field e name do nó.
bool BinaryParser::parseStructInstance(const schema::Field *definition, qint64 offset, qint64 size, QByteArrayView data, ParsedNode &out)
{
    out.offset = offset;
    out.size = size;
    out.truncated = offset + size > data.size();

    if (offset >= data.size() && offset > 0) {
        // Instância totalmente fora do arquivo: não expande os campos.
        return true;
    }

    if (!parseFields(definition, offset, data, out.children))
        return false;

    QStringList names;
    QList<format::Value> values;
    for (const ParsedNode &child : std::as_const(out.children)) {
        names << child.name;
        values << child.value;
    }

    out.value = format::Value::makeStruct(names, values, sliceOf(data, offset, size));

    return true;
}

}

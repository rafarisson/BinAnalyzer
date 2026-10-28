#include <QCoreApplication>
#include <QSet>

#include "bina/document/DocumentFactory.h"
#include "bina/document/FieldTree.h"

// bina::schema
#include "bina/schema/DataType.h"
#include "bina/schema/Identifier.h"
#include "bina/schema/Field.h"
#include "bina/schema/Types.h"
#include "bina/schema/LayoutCalculator.h"
#include "bina/schema/Builtins.h"

// bina::format
#include "bina/format/FieldFormat.h"
#include "bina/format/FormatRegistry.h"

namespace bina::document {

schema::OperationResult DocumentFactory::create(const DocumentDto &data, Document &out)
{
    using schema::OperationResult;

    m_error.clear();

    if (data.version < 1 || data.version > DocumentDto::CurrentVersion) {
        return OperationResult::failure(QCoreApplication::translate("bina", "Versão %1 não suportada (a mais recente é %2)")
                                            .arg(data.version).arg(DocumentDto::CurrentVersion));
    }

    if (const OperationResult valid = schema::Alignment::validateMax(data.maxAlign); !valid)
        return valid;

    auto registry = schema::TypeRegistry::withBuiltins();

    auto formats = std::make_unique<format::FormatRegistry>();

    for (const DocumentFormatterDto &f : data.formatters) {
        if (const OperationResult added = formats->addPattern(f.id, f.pattern); !added)
            return added;
    }

    QStringList headerIds;

    for (const DocumentTypeDto &type : data.types) {
        if (!validateNames(type.definition))
            return OperationResult::failure(m_error);

        if (const OperationResult added = registry->add(type.id, buildField(type.definition), type.defaultFormatter); !added)
            return added;

        const format::Target typeTarget{schema::Representation::None, 1, true};
        if (!type.defaultFormatter.isEmpty() && !formats->isCompatible(type.defaultFormatter, typeTarget)) {
            return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' usa o formato desconhecido ou incompatível '%2'")
                                                .arg(type.id, type.defaultFormatter));
        }

        if (!validateFormats(*registry->definition(schema::normalizeIdentifier(type.id)), *registry, *formats))
            return OperationResult::failure(m_error);

        if (type.header)
            headerIds << schema::normalizeIdentifier(type.id);
    }

    const QString activeHeader = data.header.isEmpty() ? QString() : schema::normalizeIdentifier(data.header);

    if (!activeHeader.isEmpty() && !headerIds.contains(activeHeader)) {
        return OperationResult::failure(QCoreApplication::translate("bina", "O cabeçalho '%1' não é um tipo marcado como cabeçalho")
                                            .arg(data.header));
    }

    if (!validateNames(data.root))
        return OperationResult::failure(m_error);

    // O mesmo layout dos tipos vale para a raiz: todos os campos devem usar tipos registrados.
    std::unique_ptr<schema::Field> root = buildField(data.root);
    if (schema::LayoutCalculator(registry.get(), data.maxAlign).structLayout(root.get()).size <= 0
        && root->childCount() > 0) {
        return OperationResult::failure(QCoreApplication::translate("bina", "Os campos do documento usam tipos desconhecidos"));
    }

    if (!validateFormats(*root, *registry, *formats))
        return OperationResult::failure(m_error);

    const QSysInfo::Endian byteOrder = data.bigEndian ? QSysInfo::BigEndian : QSysInfo::LittleEndian;
    const bool byteOrderChanged = out.m_byteOrder != byteOrder;
    const bool maxAlignChanged = out.m_maxAlign != data.maxAlign;
    const bool activeHeaderChanged = out.m_activeHeader != activeHeader;

    emit out.typesAboutToChange();

    out.m_version = data.version;
    out.m_byteOrder = byteOrder;
    out.m_maxAlign = data.maxAlign;
    out.m_registry = std::move(registry);
    out.m_formats = std::move(formats);
    out.m_headerIds = headerIds;
    out.m_activeHeader = activeHeader;
    out.m_fields->setRegistry(out.m_registry.get());
    out.m_fields->setMaxAlign(data.maxAlign);
    out.m_fields->setFormatRegistry(out.m_formats.get());
    out.m_fields->setRoot(std::move(root));

    emit out.typesChanged();

    if (activeHeaderChanged)
        emit out.activeHeaderChanged();

    if (byteOrderChanged)
        emit out.byteOrderChanged();
    if (maxAlignChanged)
        emit out.maxAlignChanged();

    return OperationResult::success();
}

DocumentDto DocumentFactory::toDto(const Document &doc)
{
    DocumentDto dto;

    dto.version = DocumentDto::CurrentVersion;
    dto.maxAlign = doc.maxAlign();
    dto.bigEndian = doc.byteOrder() == QSysInfo::BigEndian;

    for (const auto &pattern : doc.formats()->patterns())
        dto.formatters.append({pattern.id, pattern.spec});

    for (const schema::DataType *type : doc.types()->types()) {
        if (!type->definition())
            continue;

        DocumentTypeDto t;
        t.id = type->id();
        t.defaultFormatter = type->formatId();
        t.header = doc.isHeader(type->id());
        t.definition = toFieldDto(*type->definition());
        dto.types.append(t);
    }

    dto.header = doc.activeHeader();

    if (doc.fields()->root())
        dto.root = toFieldDto(*doc.fields()->root());

    return dto;
}

// Nomes dos campos: sem repetição entre irmãos.
bool DocumentFactory::validateNames(const DocumentFieldDto &parent)
{
    QSet<QString> seen;

    for (const DocumentFieldDto &child : parent.children) {
        const QString name = schema::normalizeIdentifier(child.name);

        if (seen.contains(name)) {
            m_error = QCoreApplication::translate("bina", "Campo '%1' repetido em '%2'").arg(name, parent.name);
            return false;
        }

        seen.insert(name);

        if (child.count < 1 || child.count > schema::Field::kMaxCount) {
            m_error = QCoreApplication::translate("bina", "'count' inválido em '%1'").arg(name);
            return false;
        }

        if (!validateNames(child))
            return false;
    }

    return true;
}

// Um formato explícito deve existir e ser compatível com o tipo e o count do campo.
bool DocumentFactory::validateFormats(const schema::Field &field, const schema::TypeRegistry &types, const format::FormatRegistry &formats)
{
    if (!format::isDisplayable(field))
        return true;

    if (!format::isFormatValid(field, types, formats, field.formatId())) {
        m_error = QCoreApplication::translate("bina", "O campo '%1' usa o formato desconhecido ou incompatível '%2'")
                      .arg(field.name(), field.formatId());
        return false;
    }

    for (int i = 0; i < field.childCount(); ++i)
        if (!validateFormats(*field.child(i), types, formats))
            return false;

    return true;
}

std::unique_ptr<schema::Field> DocumentFactory::buildField(const DocumentFieldDto &data)
{
    auto field = std::make_unique<schema::Field>(data.name);

    // Campos com filhos são sempre do tipo struct.
    field->setTypeId(data.children.isEmpty() ? data.type : QString(schema::PrimitiveTypes::kStruct));
    field->setCount(data.count);
    field->setFormatId(data.formatter);

    for (int i = 0; i < data.children.size(); ++i)
        field->insertExistingChild(i, buildField(data.children.at(i)));

    return field;
}

DocumentFieldDto DocumentFactory::toFieldDto(const schema::Field &field)
{
    DocumentFieldDto dto;

    dto.name = field.name();
    dto.type = field.typeId();
    dto.formatter = field.formatId();
    dto.count = field.count();

    for (int i = 0; i < field.childCount(); ++i) {
        const schema::Field *child = field.child(i);
        if (!child->isAutoPad())
            dto.children.append(toFieldDto(*child));
    }

    return dto;
}

} // namespace bina::document

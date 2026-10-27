#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "bina/io/JsonDocumentImporter.h"
#include "bina/io/JsonKeys.h"
#include "bina/schema/Builtins.h"
#include "bina/schema/Field.h"

namespace bina::io {

bool JsonDocumentImporter::readDevice(QIODevice *device, document::DocumentDto &out)
{
    if (!device || !device->isReadable()) {
        fail(QCoreApplication::translate("bina", "Não é possível ler o dispositivo"));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(device->readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        fail(QCoreApplication::translate("bina", "Posição %1: %2").arg(parseError.offset).arg(parseError.errorString()));
        return false;
    }

    if (!json.isObject()) {
        fail(QCoreApplication::translate("bina", "A raiz do documento deve ser um objeto"));
        return false;
    }

    if (!readDocument(json.object(), out)) {
        out = document::DocumentDto();
        return false;
    }

    return true;
}

bool JsonDocumentImporter::readDocument(const QJsonObject &obj, document::DocumentDto &out)
{
    using namespace JsonKeys;

    const int version = obj.value(kVersion).toInt(0);

    if (version < 1) {
        fail(QCoreApplication::translate("bina", "'version' inválida"));
        return false;
    }
    if (version > document::DocumentDto::CurrentVersion) {
        fail(QCoreApplication::translate("bina", "O arquivo é da versão %1, mais nova que a versão suportada (%2)")
                 .arg(version).arg(document::DocumentDto::CurrentVersion));
        return false;
    }
    out.version = version;

    if (obj.contains(kMaxAlign)) {
        out.maxAlign = obj.value(kMaxAlign).toInt(0);
        if (out.maxAlign < 1) {
            fail(QCoreApplication::translate("bina", "'maxAlign' inválido"));
            return false;
        }
    }

    if (obj.contains(kByteOrder)) {
        const QString order = obj.value(kByteOrder).toString();
        if (order != QLatin1String("little") && order != QLatin1String("big")) {
            fail(QStringLiteral("Invalid 'byteOrder': expected \"little\" or \"big\""));
            return false;
        }
        out.bigEndian = order == QLatin1String("big");
    }

    for (auto it = obj.begin(); it != obj.end(); ++it) {
        const QString key = it.key();
        if (key != kVersion && key != kMaxAlign && key != kByteOrder && key != kFormatters
            && key != kTypes && key != kElements)
            addWarning(QCoreApplication::translate("bina", "Chave desconhecida '%1' ignorada").arg(key));
    }

    return readFormatters(obj, out) && readTypes(obj, out) && readFields(obj, out.root, 1);
}

bool JsonDocumentImporter::readFormatters(const QJsonObject &obj, document::DocumentDto &out)
{
    using namespace JsonKeys;

    if (!obj.contains(kFormatters))
        return true;

    if (!obj.value(kFormatters).isArray()) {
        fail(QCoreApplication::translate("bina", "'formatters' deve ser uma lista"));
        return false;
    }

    for (const QJsonValue &value : obj.value(kFormatters).toArray()) {
        const QJsonObject o = value.toObject();

        document::DocumentFormatterDto f;
        f.id = o.value(kId).toString();
        f.pattern = o.value(kPattern).toString();

        if (!value.isObject() || f.id.isEmpty() || f.pattern.isEmpty()) {
            fail(QCoreApplication::translate("bina", "Um formato precisa de 'id' e 'pattern'"));
            return false;
        }

        out.formatters.append(f);
    }

    return true;
}

bool JsonDocumentImporter::readTypes(const QJsonObject &obj, document::DocumentDto &out)
{
    using namespace JsonKeys;

    if (!obj.contains(kTypes))
        return true;

    if (!obj.value(kTypes).isArray()) {
        fail(QCoreApplication::translate("bina", "'types' deve ser uma lista"));
        return false;
    }

    for (const QJsonValue &value : obj.value(kTypes).toArray()) {
        if (!value.isObject()) {
            fail(QCoreApplication::translate("bina", "Cada item de 'types' deve ser um objeto"));
            return false;
        }

        const QJsonObject o = value.toObject();
        document::DocumentTypeDto t;

        t.id = o.value(kId).toString();
        t.defaultFormatter = o.value(kFormatter).toString();

        if (t.id.isEmpty()) {
            fail(QCoreApplication::translate("bina", "Um tipo precisa de 'id'"));
            return false;
        }

        if (!readFields(o, t.definition, 1))
            return false;

        out.types.append(t);
    }

    return true;
}

bool JsonDocumentImporter::readFields(const QJsonObject &obj, document::DocumentFieldDto &parent, int depth)
{
    using namespace JsonKeys;

    if (!obj.contains(kElements))
        return true;

    if (!obj.value(kElements).isArray()) {
        fail(QCoreApplication::translate("bina", "'elements' deve ser uma lista"));
        return false;
    }

    if (depth > MaxDepth) {
        fail(QCoreApplication::translate("bina", "O aninhamento passa de %1 níveis").arg(MaxDepth));
        return false;
    }

    for (const QJsonValue &value : obj.value(kElements).toArray()) {
        if (!value.isObject()) {
            fail(QCoreApplication::translate("bina", "Cada item de 'elements' deve ser um objeto"));
            return false;
        }

        document::DocumentFieldDto e;
        if (!readField(value.toObject(), e, depth))
            return false;

        parent.children.append(e);
    }

    return true;
}

bool JsonDocumentImporter::readField(const QJsonObject &obj, document::DocumentFieldDto &out, int depth)
{
    using namespace JsonKeys;

    out.name = obj.value(kName).toString();
    out.type = obj.value(kType).toString();
    out.formatter = obj.value(kFormatter).toString();

    const bool hasChildren = obj.contains(kElements);

    // Campos com filhos são do tipo struct; o 'type' pode ser omitido.
    if (hasChildren && out.type.isEmpty())
        out.type = schema::PrimitiveTypes::kStruct;

    if (out.name.isEmpty() || out.type.isEmpty()) {
        fail(QCoreApplication::translate("bina", "Um campo precisa de 'name' e 'type'"));
        return false;
    }

    if (hasChildren != (out.type == schema::PrimitiveTypes::kStruct)) {
        fail(QCoreApplication::translate("bina", "Campo '%1': o tipo 'struct' é usado se e somente se o campo tem 'elements'").arg(out.name));
        return false;
    }

    if (obj.contains(kCount)) {
        out.count = obj.value(kCount).toInt(0);
        if (out.count < 1 || out.count > schema::Field::kMaxCount) {
            fail(QCoreApplication::translate("bina", "'count' inválido em '%1'").arg(out.name));
            return false;
        }
    }

    return readFields(obj, out, depth + 1);
}

} // namespace bina::io

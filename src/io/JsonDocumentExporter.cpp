#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "bina/io/JsonDocumentExporter.h"
#include "bina/io/JsonKeys.h"

namespace bina::io {

namespace {

QJsonArray fieldsToJson(const QList<document::DocumentFieldDto> &elements);

QJsonObject fieldToJson(const document::DocumentFieldDto &e)
{
    using namespace JsonKeys;

    QJsonObject obj;

    obj[kName] = e.name;
    obj[kType] = e.type;
    if (e.count != 1)
        obj[kCount] = e.count;
    if (!e.formatter.isEmpty())
        obj[kFormatter] = e.formatter;
    if (!e.children.isEmpty())
        obj[kElements] = fieldsToJson(e.children);

    return obj;
}

QJsonArray fieldsToJson(const QList<document::DocumentFieldDto> &elements)
{
    QJsonArray array;
    for (const document::DocumentFieldDto &e : elements)
        array.append(fieldToJson(e));
    return array;
}

}

bool JsonDocumentExporter::write(QIODevice *device, const document::DocumentDto &data)
{
    using namespace JsonKeys;

    QJsonObject obj;

    obj[kVersion] = data.version;
    obj[kMaxAlign] = data.maxAlign;
    obj[kByteOrder] = data.bigEndian ? QStringLiteral("big") : QStringLiteral("little");

    QJsonArray formatters;
    for (const document::DocumentFormatterDto &f : data.formatters)
        formatters.append(QJsonObject{{kId, f.id}, {kPattern, f.pattern}});

    if (!formatters.isEmpty())
        obj[kFormatters] = formatters;

    QJsonArray types;
    for (const document::DocumentTypeDto &t : data.types) {
        QJsonObject o;
        o[kId] = t.id;
        if (!t.defaultFormatter.isEmpty())
            o[kFormatter] = t.defaultFormatter;
        o[kElements] = fieldsToJson(t.definition.children);
        types.append(o);
    }

    obj[kTypes] = types;
    obj[kElements] = fieldsToJson(data.root.children);

    return writeAll(device, QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

} // namespace bina::io

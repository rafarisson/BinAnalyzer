#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "bina/io/ResultExporters.h"

namespace bina::io {

namespace {

QJsonArray nodesToJson(const QList<ResultNodeDto> &nodes)
{
    QJsonArray array;

    for (const ResultNodeDto &n : nodes) {
        QJsonObject obj{
            {QStringLiteral("name"), n.name},
            {QStringLiteral("type"), n.type},
            {QStringLiteral("offset"), n.offset},
            {QStringLiteral("bytes"), n.bytes},
            {QStringLiteral("value"), n.value}
        };

        if (!n.children.isEmpty())
            obj.insert(QStringLiteral("children"), nodesToJson(n.children));

        array.append(obj);
    }

    return array;
}

// Só as células que confundiriam a leitura (com o separador, aspas ou quebra de linha) vão entre
// aspas, com as aspas internas dobradas; as demais saem como estão.
QString csvCell(const QString &text, QChar separator)
{
    if (!text.contains(separator) && !text.contains(QLatin1Char('"'))
        && !text.contains(QLatin1Char('\n')) && !text.contains(QLatin1Char('\r')))
        return text;

    QString quoted = text;
    quoted.replace(QLatin1Char('"'), QStringLiteral("\"\""));

    return QLatin1Char('"') + quoted + QLatin1Char('"');
}

QString csvRow(const QStringList &cells, QChar separator)
{
    QStringList out;
    for (const QString &cell : cells)
        out << csvCell(cell, separator);

    return out.join(separator) + QStringLiteral("\r\n");
}

}

bool JsonResultExporter::write(QIODevice *device, const ResultTreeDto &data)
{
    return writeAll(device, QJsonDocument(nodesToJson(data.nodes)).toJson(QJsonDocument::Indented));
}

CsvResultExporter::CsvResultExporter(QChar separator)
    : m_separator{separator}
{
}

bool CsvResultExporter::write(QIODevice *device, const ResultTableDto &data)
{
    QString text = csvRow(data.headers, m_separator);
    for (const QStringList &row : data.rows)
        text += csvRow(row, m_separator);

    return writeAll(device, QByteArray("\xEF\xBB\xBF") + text.toUtf8());
}

} // namespace bina::io

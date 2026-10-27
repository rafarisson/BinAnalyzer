#include <QFileInfo>

#include "bina/io/Codecs.h"
#include "bina/io/JsonDocumentExporter.h"
#include "bina/io/JsonDocumentImporter.h"
#include "bina/io/ResultExporters.h"

namespace bina::io {

namespace {

QString suffixOf(const QString &path)
{
    return QFileInfo(path).suffix().toLower();
}

template <typename Codec>
const Codec *findBySuffix(const QList<Codec> &codecs, const QString &path)
{
    const QString suffix = suffixOf(path);

    for (const Codec &codec : codecs)
        if (codec.suffix == suffix)
            return &codec;

    return nullptr;
}

}

DocumentCodecRegistry DocumentCodecRegistry::withBuiltins()
{
    DocumentCodecRegistry registry;

    registry.add({
        QStringLiteral("json"),
        QStringLiteral("JSON"),
        [] { return std::make_unique<JsonDocumentImporter>(); },
        [] { return std::make_unique<JsonDocumentExporter>(); }
    });

    return registry;
}

void DocumentCodecRegistry::add(const DocumentCodec &codec)
{
    m_codecs.append(codec);
}

const DocumentCodec *DocumentCodecRegistry::forPath(const QString &path) const
{
    return findBySuffix(m_codecs, path);
}

QString DocumentCodecRegistry::defaultSuffix() const
{
    return m_codecs.isEmpty() ? QString() : m_codecs.first().suffix;
}

ResultCodecRegistry ResultCodecRegistry::withBuiltins()
{
    ResultCodecRegistry registry;

    registry.add({
        QStringLiteral("json"),
        QStringLiteral("JSON"),
        ResultCodec::Tree,
        [] { return std::make_unique<JsonResultExporter>(); },
        {}
    });

    registry.add({
        QStringLiteral("csv"),
        QStringLiteral("CSV"),
        ResultCodec::Table,
        {},
        [] { return std::make_unique<CsvResultExporter>(); }
    });

    return registry;
}

void ResultCodecRegistry::add(const ResultCodec &codec)
{
    m_codecs.append(codec);
}

const ResultCodec *ResultCodecRegistry::forPath(const QString &path) const
{
    return findBySuffix(m_codecs, path);
}

QString ResultCodecRegistry::defaultSuffix() const
{
    return m_codecs.isEmpty() ? QString() : m_codecs.first().suffix;
}

QStringList ResultCodecRegistry::suffixes() const
{
    QStringList out;
    for (const ResultCodec &codec : m_codecs)
        out << codec.suffix;
    return out;
}

} // namespace bina::io

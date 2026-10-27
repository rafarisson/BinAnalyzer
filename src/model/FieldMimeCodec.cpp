#include <QDataStream>
#include <QIODevice>

#include "bina/model/FieldMimeCodec.h"
#include "bina/model/FieldRef.h"

namespace bina::model {

namespace {

inline const auto FieldMimeType = QStringLiteral("application/x-bina-element");

constexpr quint32 Magic = 0x42494E41;   // 'BINA'
constexpr quint16 Version = 2;     // 2: o campo é conferido pelo id, não mais pelo nome|tipo|count
constexpr qsizetype MaxPayload = 4096;
constexpr qint32 MaxRow = 1 << 20;

}

QMimeData *FieldMimeCodec::encode(const FieldRef &ref)
{
    if (!ref.isValid() || ref.path.size() > MaxPathDepth)
        return nullptr;

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);

    stream << Magic << Version << ref.modelToken << ref.revision << ref.fieldId
           << quint16(ref.path.size());

    for (const int row : ref.path)
        stream << qint32(row);

    if (stream.status() != QDataStream::Ok)
        return nullptr;

    auto *mime = new QMimeData;
    mime->setData(FieldMimeType, data);

    return mime;
}

bool FieldMimeCodec::decode(const QMimeData *mime, FieldRef &ref)
{
    if (!canDecode(mime))
        return false;

    const QByteArray data = mime->data(FieldMimeType);
    if (data.isEmpty() || data.size() > MaxPayload)
        return false;

    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_0);

    quint32 magic = 0;
    quint16 version = 0;
    FieldRef decoded;

    stream >> magic >> version;
    if (stream.status() != QDataStream::Ok || magic != Magic || version != Version)
        return false;

    quint16 depth = 0;
    stream >> decoded.modelToken >> decoded.revision >> decoded.fieldId >> depth;

    if (stream.status() != QDataStream::Ok || depth == 0 || depth > MaxPathDepth)
        return false;

    for (quint16 i = 0; i < depth; ++i) {
        qint32 row = -1;
        stream >> row;

        if (stream.status() != QDataStream::Ok || row < 0 || row > MaxRow)
            return false;

        decoded.path.append(row);
    }

    // Dados sobrando indicam um payload que não foi gerado por encode().
    if (!stream.atEnd() || !decoded.isValid())
        return false;

    ref = decoded;

    return true;
}

bool FieldMimeCodec::canDecode(const QMimeData *mime)
{
    return mime && mime->hasFormat(FieldMimeType);
}

QStringList FieldMimeCodec::mimeTypes()
{
    return { FieldMimeType };
}

} // namespace bina::model

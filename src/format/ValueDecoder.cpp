#include <cstring>

#include <QStringDecoder>

#include "bina/format/ValueDecoder.h"

namespace bina::format {

namespace {

quint64 readBits(QByteArrayView bytes, QSysInfo::Endian endian)
{
    quint64 bits = 0;
    for (qsizetype i = 0; i < bytes.size(); ++i) {
        const qsizetype pos = endian == QSysInfo::LittleEndian ? bytes.size() - 1 - i : i;
        bits = (bits << 8) | quint8(bytes.at(pos));
    }
    return bits;
}

qint64 signExtend(quint64 bits, int size)
{
    if (size >= 8)
        return qint64(bits);

    const quint64 sign = quint64(1) << (size * 8 - 1);
    return qint64((bits ^ sign) - sign);
}

Value decodeScalar(Representation representation, QByteArrayView bytes, QSysInfo::Endian endian)
{
    const int size = int(bytes.size());
    const QByteArray raw = bytes.toByteArray();

    if (size < 1 || size > 8)
        return Value::makeBytes(raw);

    const quint64 bits = readBits(bytes, endian);

    switch (representation) {
    case Representation::Signed:
        return Value::makeSigned(signExtend(bits, size), size, representation, raw);

    case Representation::Float:
        if (size == 4) {
            const quint32 b32 = quint32(bits);
            float f;
            std::memcpy(&f, &b32, sizeof f);
            return Value::makeFloat(f, size, raw);
        }
        if (size == 8) {
            double d;
            std::memcpy(&d, &bits, sizeof d);
            return Value::makeFloat(d, size, raw);
        }
        return Value::makeBytes(raw);

    case Representation::Unsigned:
    case Representation::Char:
    case Representation::Enum:
        return Value::makeUnsigned(bits, size, representation, raw);

    default:
        return Value::makeBytes(raw);
    }
}

}

Value decodeValue(Representation representation, int fieldSize, int count, QByteArrayView bytes, QSysInfo::Endian endian)
{
    if (fieldSize < 1 || count < 1 || bytes.size() < qsizetype(fieldSize) * count)
        return {};

    bytes = bytes.first(qsizetype(fieldSize) * count);

    if (count == 1)
        return decodeScalar(representation, bytes, endian);

    const QByteArray raw = bytes.toByteArray();

    if (representation == Representation::Char && fieldSize == 1) {
        // O texto termina no primeiro NUL; os bytes originais permanecem em raw().
        const qsizetype nul = raw.indexOf('\0');
        return Value::makeText(decodeText(QByteArrayView(raw).first(nul < 0 ? raw.size() : nul)), raw);
    }

    if (representation == Representation::Opaque || representation == Representation::None)
        return Value::makeBytes(raw);

    QList<Value> items;
    items.reserve(count);
    for (int i = 0; i < count; ++i)
        items.append(decodeScalar(representation, bytes.sliced(qsizetype(i) * fieldSize, fieldSize), endian));

    return Value::makeArray(items, raw);
}

QString decodeText(QByteArrayView bytes)
{
    QStringDecoder utf8(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    const QString text = utf8.decode(bytes);

    return utf8.hasError() ? QString::fromLatin1(bytes) : text;
}

}

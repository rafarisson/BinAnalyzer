#include <cstdio>
#include <limits>

#include <QCoreApplication>
#include <QRegularExpression>

#include "bina/format/PrintfFormat.h"
#include "bina/format/ValueDecoder.h"

namespace bina::format {

namespace {

constexpr int MaxWidth = 256;

const QString kMissing = QStringLiteral("<?>");

QString pad(QString text, const QString &flags, int width)
{
    if (width <= text.size())
        return text;

    return flags.contains(QLatin1Char('-'))
               ? text.leftJustified(width, QLatin1Char(' '))
               : text.rightJustified(width, QLatin1Char(' '));
}

template <typename T>
QString cFormat(const QString &flags, int width, int precision, const char *length, char conversion, T value)
{
    QByteArray fmt = "%" + flags.toLatin1();
    if (width >= 0)
        fmt += QByteArray::number(width);
    if (precision >= 0)
        fmt += '.' + QByteArray::number(precision);
    fmt += length;
    fmt += conversion;

    const int size = std::snprintf(nullptr, 0, fmt.constData(), value);
    if (size < 0)
        return {};

    QByteArray buffer(size + 1, '\0');
    std::snprintf(buffer.data(), buffer.size(), fmt.constData(), value);

    return QString::fromLatin1(buffer.constData(), size);
}

// Bits do valor somado ao deslocamento, limitados à largura do tipo (complemento de dois): "%x" de
// um uint8 0x01 com "-2" dá 0xff, não 0xffffffffffffffff.
quint64 offsetBits(const Value &arg, qint64 offset)
{
    const quint64 bits = arg.toRawBits() + quint64(offset);
    const int size = arg.size();

    return size > 0 && size < 8 ? bits & ((quint64(1) << (size * 8)) - 1) : bits;
}

QString textOf(const Value &arg)
{
    switch (arg.type()) {
    case Value::Text:       return arg.text();
    case Value::Bytes:      return decodeText(arg.raw());
    case Value::Signed:     return QString::number(arg.toSigned());
    case Value::Unsigned:   return arg.representation() == Representation::Char
                                       ? QString(QChar(char16_t(arg.toUnsigned() & 0xFF)))
                                       : QString::number(arg.toUnsigned());
    case Value::Float:      return QString::number(arg.toDouble(), 'g', 17);
    default:                return {};
    }
}

}

PrintfFormat PrintfFormat::compile(const QString &spec)
{
    static const QRegularExpression keyRe(QStringLiteral(R"(^\s*([\p{L}_][\p{L}\p{N}_.]*)\s*(?:([+-])\s*(\d+))?\s*$)"));
    static const QString validConversions = QStringLiteral("diuxXobcsfFeEgG");

    PrintfFormat out;
    out.m_spec = spec;

    QString literal;
    qsizetype i = 0;

    auto flushLiteral = [&] {
        if (literal.isEmpty())
            return;
        Segment s;
        s.literal = literal;
        out.m_segments.append(s);
        literal.clear();
    };

    auto fail = [&](const QString &message) {
        out.m_error = QStringLiteral("%1 (position %2)").arg(message).arg(i);
        out.m_segments.clear();
        return out;
    };

    while (i < spec.size()) {
        const QChar ch = spec.at(i++);

        if (ch != QLatin1Char('%')) {
            literal += ch;
            continue;
        }

        if (i >= spec.size())
            return fail(QCoreApplication::translate("bina", "Conversão incompleta"));

        if (spec.at(i) == QLatin1Char('%')) {
            literal += QLatin1Char('%');
            ++i;
            continue;
        }

        flushLiteral();

        Segment seg;
        seg.isConversion = true;

        if (spec.at(i) == QLatin1Char('(')) {
            const qsizetype end = spec.indexOf(QLatin1Char(')'), i);
            if (end < 0)
                return fail(QCoreApplication::translate("bina", "Falta ')'"));

            const auto match = keyRe.match(spec.mid(i + 1, end - i - 1));
            if (!match.hasMatch())
                return fail(QCoreApplication::translate("bina", "Referência de campo inválida"));

            seg.key = match.captured(1);
            if (!match.captured(3).isEmpty()) {
                const qint64 n = match.captured(3).toLongLong();
                seg.offset = match.captured(2) == QLatin1String("-") ? -n : n;
            }

            i = end + 1;
        }

        while (i < spec.size() && QStringLiteral("-+ #0").contains(spec.at(i)))
            seg.flags += spec.at(i++);

        auto readNumber = [&](int &number) {
            if (i >= spec.size() || !spec.at(i).isDigit())
                return true;

            qint64 n = 0;
            while (i < spec.size() && spec.at(i).isDigit()) {
                n = n * 10 + spec.at(i++).digitValue();
                if (n > MaxWidth)
                    return false;
            }
            number = int(n);
            return true;
        };

        if (!readNumber(seg.width))
            return fail(QCoreApplication::translate("bina", "Largura grande demais"));

        if (i < spec.size() && spec.at(i) == QLatin1Char('.')) {
            ++i;
            seg.precision = 0;
            if (!readNumber(seg.precision))
                return fail(QCoreApplication::translate("bina", "Precisão grande demais"));
        }

        while (i < spec.size() && QStringLiteral("hlLzjtq").contains(spec.at(i)))
            ++i;

        if (i >= spec.size())
            return fail(QCoreApplication::translate("bina", "Conversão incompleta"));

        seg.conversion = spec.at(i);
        if (!validConversions.contains(seg.conversion))
            return fail(QCoreApplication::translate("bina", "Conversão desconhecida: '%1'").arg(seg.conversion));

        ++i;
        out.m_segments.append(seg);
    }

    flushLiteral();

    return out;
}

QList<PrintfFormat::Reference> PrintfFormat::references() const
{
    QList<Reference> out;

    for (const Segment &seg : m_segments) {
        if (seg.isConversion)
            out.append({seg.key, seg.conversion});
    }

    return out;
}

QString PrintfFormat::format(const Value &value) const
{
    if (!isValid())
        return {};

    const QList<Value> leaves = value.leaves();
    qsizetype next = 0;

    QString out;

    for (const Segment &seg : m_segments) {
        if (!seg.isConversion) {
            out += seg.literal;
            continue;
        }

        const Value *arg = nullptr;
        Value holder;

        if (!seg.key.isEmpty()) {
            arg = value.field(seg.key);
            if (arg && (arg->type() == Value::Struct || arg->type() == Value::Array)) {
                holder = arg->leaves().value(0);
                arg = &holder;
            }
        } else if (next < leaves.size()) {
            arg = &leaves.at(next++);
        }

        out += arg && arg->isValid() ? render(seg, *arg) : kMissing;
    }

    return out;
}

QString PrintfFormat::render(const Segment &seg, const Value &arg) const
{
    const QChar conv = seg.conversion;
    const char c = conv.toLatin1();

    switch (c) {
    case 'd':
    case 'i':
        // Um uint64 acima de INT64_MAX não cabe em um inteiro com sinal: sai sem sinal.
        if (arg.type() == Value::Unsigned && arg.toUnsigned() > quint64(std::numeric_limits<qint64>::max()))
            return cFormat(seg.flags, seg.width, seg.precision, "ll", 'u', qulonglong(arg.toUnsigned() + quint64(seg.offset)));

        return cFormat(seg.flags, seg.width, seg.precision, "ll", 'd', qlonglong(arg.toSigned() + seg.offset));

    case 'u':
    case 'x':
    case 'X':
    case 'o':
        return cFormat(seg.flags, seg.width, seg.precision, "ll", c, qulonglong(offsetBits(arg, seg.offset)));

    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
        return cFormat(seg.flags, seg.width, seg.precision, "", c, arg.toDouble() + double(seg.offset));

    case 'b': {
        QString digits = QString::number(offsetBits(arg, seg.offset), 2);
        if (seg.precision > digits.size())
            digits = digits.rightJustified(seg.precision, QLatin1Char('0'));

        const QString prefix = seg.flags.contains(QLatin1Char('#')) ? QStringLiteral("0b") : QString();

        // O preenchimento com zeros vem depois do prefixo ("0b00000101").
        if (seg.flags.contains(QLatin1Char('0')) && !seg.flags.contains(QLatin1Char('-')) && seg.precision < 0)
            digits = digits.rightJustified(qMax(0, seg.width - int(prefix.size())), QLatin1Char('0'));

        return pad(prefix + digits, seg.flags, seg.width);
    }

    case 'c':
        return pad(QString(QChar(char16_t(arg.toUnsigned() + quint64(seg.offset)))), seg.flags, seg.width);

    case 's': {
        QString text = textOf(arg);
        if (seg.precision >= 0)
            text.truncate(seg.precision);
        return pad(text, seg.flags, seg.width);
    }

    default:
        return {};
    }
}

PrintfFormatter::PrintfFormatter(const QString &id, const PrintfFormat &format)
    : m_id{id}
    , m_format{format}
{
}

QString PrintfFormatter::format(const Value &value) const
{
    // Array de structs: aplica o padrão a cada elemento.
    if (value.type() == Value::Array && !value.items().isEmpty() && value.items().first().type() == Value::Struct) {
        QStringList parts;
        for (const Value &item : value.items())
            parts << m_format.format(item);

        return QLatin1Char('[') + parts.join(QStringLiteral(", ")) + QLatin1Char(']');
    }

    return m_format.format(value);
}

}

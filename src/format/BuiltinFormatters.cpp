#include <charconv>

#include <QCoreApplication>

#include "bina/format/BuiltinFormatters.h"

namespace bina::format {

namespace {

bool isIntegerRepresentation(Representation representation)
{
    return representation == Representation::Signed || representation == Representation::Unsigned
           || representation == Representation::Enum || representation == Representation::Char;
}

class IntegerFormatter : public Formatter
{
public:

    enum Radix { Decimal, Hex, Octal, Binary };

    IntegerFormatter(Radix radix, const QString &id, const QString &name)
        : m_radix{radix}, m_id{id}, m_name{name}
    {}

    QString id() const override { return m_id; }
    QString displayName() const override { return m_name; }

    bool accepts(const Target &target) const override
    {
        return !target.isStruct && isIntegerRepresentation(target.representation);
    }

protected:

    QString formatScalar(const Value &value) const override
    {
        if (!value.isInteger())
            return {};

        const int size = value.size() > 0 ? value.size() : 8;

        switch (m_radix) {
        case Decimal:
            return value.type() == Value::Signed
                       ? QString::number(value.toSigned())
                       : QString::number(value.toUnsigned());
        case Hex:
            return QStringLiteral("0x") + QString::number(value.toRawBits(), 16).toUpper()
                       .rightJustified(size * 2, QLatin1Char('0'));
        case Octal:
            return QLatin1Char('0') + QString::number(value.toRawBits(), 8);
        case Binary:
            return QStringLiteral("0b") + QString::number(value.toRawBits(), 2)
                       .rightJustified(size * 8, QLatin1Char('0'));
        }

        return {};
    }

private:

    Radix m_radix;
    QString m_id;
    QString m_name;
};

class FloatFormatter : public Formatter
{
public:

    QString id() const override { return FormatIds::kFloat; }
    QString displayName() const override { return QCoreApplication::translate("bina", "Real"); }

    bool accepts(const Target &target) const override
    {
        return !target.isStruct && target.representation == Representation::Float;
    }

protected:

    QString formatScalar(const Value &value) const override
    {
        if (value.type() != Value::Float)
            return {};

        // Menor representação que preserva o valor, de acordo com a precisão do tipo.
        char buffer[64];
        const auto result = value.size() == 4
                                ? std::to_chars(buffer, buffer + sizeof buffer, float(value.toDouble()))
                                : std::to_chars(buffer, buffer + sizeof buffer, value.toDouble());

        return QString::fromLatin1(buffer, result.ptr - buffer);
    }
};

class StringFormatter : public Formatter
{
public:

    QString id() const override { return FormatIds::kString; }
    QString displayName() const override { return QCoreApplication::translate("bina", "Texto"); }

    bool accepts(const Target &target) const override
    {
        return !target.isStruct && target.representation == Representation::Char;
    }

    QString format(const Value &value) const override
    {
        if (value.type() == Value::Text)
            return QLatin1Char('"') + escapeText(value.text()) + QLatin1Char('"');

        return Formatter::format(value);
    }

protected:

    QString formatScalar(const Value &value) const override
    {
        return value.isInteger()
                   ? QLatin1Char('\'') + escapeText(QString(QChar(char16_t(value.toUnsigned() & 0xFF)))) + QLatin1Char('\'')
                   : QString();
    }
};

// Buffers: "DE AD BE EF".
class BytesFormatter : public Formatter
{
public:

    QString id() const override { return FormatIds::kBytes; }
    QString displayName() const override { return QCoreApplication::translate("bina", "Bytes"); }

    bool accepts(const Target &) const override { return true; }

    QString format(const Value &value) const override
    {
        return QString::fromLatin1(value.raw().toHex(' ').toUpper());
    }

protected:

    QString formatScalar(const Value &value) const override { return format(value); }
};

// Campo avaliado mas não exibido (o resultado não o mostra): serve a qualquer campo, inclusive grupos.
class HiddenFormatter : public Formatter
{
public:

    QString id() const override { return FormatIds::kHidden; }
    QString displayName() const override { return QCoreApplication::translate("bina", "Oculto"); }

    bool accepts(const Target &) const override { return true; }

    QString format(const Value &) const override { return {}; }

protected:

    QString formatScalar(const Value &) const override { return {}; }
};

}

QString escapeText(const QString &text)
{
    QString out;
    out.reserve(text.size());

    for (const QChar ch : text) {
        switch (ch.unicode()) {
        case '\n': out += QStringLiteral("\\n"); break;
        case '\r': out += QStringLiteral("\\r"); break;
        case '\t': out += QStringLiteral("\\t"); break;
        case '\\': out += QStringLiteral("\\\\"); break;
        case '"':  out += QStringLiteral("\\\""); break;
        default:
            if (ch.unicode() < 0x20 || ch.unicode() == 0x7F || !ch.isPrint())
                out += QStringLiteral("\\x") + QString::number(ch.unicode(), 16).toUpper().rightJustified(2, QLatin1Char('0'));
            else
                out += ch;
        }
    }

    return out;
}

std::vector<std::unique_ptr<Formatter>> createBuiltinFormatters()
{
    using R = IntegerFormatter::Radix;

    std::vector<std::unique_ptr<Formatter>> out;

    out.push_back(std::make_unique<IntegerFormatter>(R::Decimal, FormatIds::kDecimal, QCoreApplication::translate("bina", "Decimal")));
    out.push_back(std::make_unique<IntegerFormatter>(R::Hex, FormatIds::kHex, QCoreApplication::translate("bina", "Hexadecimal")));
    out.push_back(std::make_unique<IntegerFormatter>(R::Octal, FormatIds::kOctal, QCoreApplication::translate("bina", "Octal")));
    out.push_back(std::make_unique<IntegerFormatter>(R::Binary, FormatIds::kBinary, QCoreApplication::translate("bina", "Binário")));
    out.push_back(std::make_unique<FloatFormatter>());
    out.push_back(std::make_unique<StringFormatter>());
    out.push_back(std::make_unique<BytesFormatter>());
    out.push_back(std::make_unique<HiddenFormatter>());

    return out;
}

}

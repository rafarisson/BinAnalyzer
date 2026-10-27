#include "bina/format/Formatter.h"

namespace bina::format {

QString Formatter::format(const Value &value) const
{
    switch (value.type()) {
    case Value::Signed:
    case Value::Unsigned:
    case Value::Float:
        return formatScalar(value);

    case Value::Array:
    case Value::Text:
    case Value::Bytes: {
        QList<Value> items = value.items();

        if (value.type() != Value::Array) {
            const Representation representation = value.type() == Value::Text ? Representation::Char : Representation::Unsigned;
            for (const char byte : value.raw())
                items.append(Value::makeUnsigned(quint8(byte), 1, representation));
        }

        QStringList parts;
        for (const Value &item : std::as_const(items))
            parts << formatScalar(item);

        return QLatin1Char('[') + parts.join(QStringLiteral(", ")) + QLatin1Char(']');
    }

    default:
        return {};
    }
}

}

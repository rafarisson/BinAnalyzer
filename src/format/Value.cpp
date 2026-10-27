#include <cmath>
#include <limits>

#include "bina/format/Value.h"

namespace bina::format {

Value Value::makeSigned(qint64 v, int size, Representation representation, const QByteArray &raw)
{
    Value out;
    out.m_type = Signed;
    out.m_representation = representation;
    out.m_size = size;
    out.m_bits = quint64(v);
    out.m_raw = raw;
    return out;
}

Value Value::makeUnsigned(quint64 v, int size, Representation representation, const QByteArray &raw)
{
    Value out;
    out.m_type = Unsigned;
    out.m_representation = representation;
    out.m_size = size;
    out.m_bits = v;
    out.m_raw = raw;
    return out;
}

Value Value::makeFloat(double v, int size, const QByteArray &raw)
{
    Value out;
    out.m_type = Float;
    out.m_representation = Representation::Float;
    out.m_size = size;
    out.m_double = v;
    out.m_raw = raw;
    return out;
}

Value Value::makeText(const QString &text, const QByteArray &raw)
{
    Value out;
    out.m_type = Text;
    out.m_representation = Representation::Char;
    out.m_size = 1;
    out.m_text = text;
    out.m_raw = raw;
    return out;
}

Value Value::makeBytes(const QByteArray &raw)
{
    Value out;
    out.m_type = Bytes;
    out.m_representation = Representation::Opaque;
    out.m_size = 1;
    out.m_raw = raw;
    return out;
}

Value Value::makeArray(const QList<Value> &items, const QByteArray &raw)
{
    Value out;
    out.m_type = Array;
    out.m_items = items;
    out.m_raw = raw;
    if (!items.isEmpty()) {
        out.m_representation = items.first().representation();
        out.m_size = items.first().size();
    }
    return out;
}

Value Value::makeStruct(const QStringList &names, const QList<Value> &fields, const QByteArray &raw)
{
    Value out;
    out.m_type = Struct;
    out.m_names = names;
    out.m_items = fields;
    out.m_raw = raw;
    return out;
}

namespace {

// double -> inteiro sem comportamento indefinido: NaN vale 0 e o resto satura nos limites do tipo.
// 2^63 e 2^64 são exatos em double; os limites são comparados como >= para ficarem de fora.
qint64 saturateToSigned(double d)
{
    if (std::isnan(d))
        return 0;
    if (d >= 9223372036854775808.0)
        return (std::numeric_limits<qint64>::max)();
    if (d < -9223372036854775808.0)
        return (std::numeric_limits<qint64>::min)();
    return qint64(d);
}

}

qint64 Value::toSigned() const
{
    switch (m_type) {
    case Signed:
    case Unsigned:  return qint64(m_bits);
    case Float:     return saturateToSigned(m_double);
    default:        return 0;
    }
}

quint64 Value::toUnsigned() const
{
    switch (m_type) {
    case Signed:
    case Unsigned:  return m_bits;
    // Negativos em complemento de dois, como um inteiro com sinal; acima de 2^64 satura.
    case Float:
        if (m_double >= 18446744073709551616.0)
            return (std::numeric_limits<quint64>::max)();
        return m_double >= 0 ? quint64(m_double) : quint64(saturateToSigned(m_double));
    default:        return 0;
    }
}

double Value::toDouble() const
{
    switch (m_type) {
    case Signed:    return double(qint64(m_bits));
    case Unsigned:  return double(m_bits);
    case Float:     return m_double;
    default:        return 0;
    }
}

quint64 Value::toRawBits() const
{
    const quint64 bits = toUnsigned();
    return m_size > 0 && m_size < 8 ? bits & ((quint64(1) << (m_size * 8)) - 1) : bits;
}

// Caminho com pontos ("bounds.min.x") através dos campos nomeados.
const Value *Value::field(const QString &path) const
{
    if (m_type != Struct)
        return nullptr;

    const qsizetype dot = path.indexOf(QLatin1Char('.'));
    const QString head = dot < 0 ? path : path.left(dot);

    const qsizetype i = m_names.indexOf(head);
    if (i < 0)
        return nullptr;

    const Value &child = m_items.at(i);

    return dot < 0 ? &child : child.field(path.mid(dot + 1));
}

QList<Value> Value::leaves() const
{
    if (m_type != Struct && m_type != Array)
        return {*this};

    QList<Value> out;
    for (const Value &item : m_items)
        out += item.leaves();
    return out;
}

}

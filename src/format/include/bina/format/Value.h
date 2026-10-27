#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

#include "bina/schema/Types.h"

namespace bina::format {

using schema::Representation;

// Valor já decodificado do binário, independente de como será exibido.
// Trocar a formatação não exige reanalisar o arquivo: o Value é reutilizado.
class Value
{
public:

    enum Type {
        Invalid,
        Signed,
        Unsigned,
        Float,
        Text,       // char[N]
        Bytes,      // buffer sem interpretação
        Array,      // N valores escalares do mesmo tipo
        Struct      // campos nomeados, na ordem de declaração
    };

    Value() = default;

    static Value makeSigned(qint64 v, int size, Representation representation, const QByteArray &raw = {});
    static Value makeUnsigned(quint64 v, int size, Representation representation, const QByteArray &raw = {});
    static Value makeFloat(double v, int size, const QByteArray &raw = {});
    static Value makeText(const QString &text, const QByteArray &raw);
    static Value makeBytes(const QByteArray &raw);
    static Value makeArray(const QList<Value> &items, const QByteArray &raw = {});
    static Value makeStruct(const QStringList &names, const QList<Value> &fields, const QByteArray &raw = {});

    Type type() const { return m_type; }
    bool isValid() const { return m_type != Invalid; }
    bool isInteger() const { return m_type == Signed || m_type == Unsigned; }
    bool isNumber() const { return isInteger() || m_type == Float; }

    Representation representation() const { return m_representation; }

    // Tamanho em bytes de um elemento escalar (0 se não se aplica).
    int size() const { return m_size; }

    qint64 toSigned() const;
    quint64 toUnsigned() const;
    double toDouble() const;

    // Valor inteiro limitado à largura do tipo (complemento de dois para negativos).
    quint64 toRawBits() const;

    QString text() const { return m_text; }
    QByteArray raw() const { return m_raw; }

    // Array: elementos. Struct: campos (com names()).
    const QList<Value> &items() const { return m_items; }
    const QStringList &names() const { return m_names; }
    const Value *field(const QString &path) const;

    // Escalares e textos em ordem, percorrendo structs e arrays recursivamente.
    QList<Value> leaves() const;

private:

    Type m_type{Invalid};
    Representation m_representation{Representation::None};
    int m_size{0};
    quint64 m_bits{0};
    double m_double{0};
    QString m_text;
    QByteArray m_raw;
    QList<Value> m_items;
    QStringList m_names;
};

}

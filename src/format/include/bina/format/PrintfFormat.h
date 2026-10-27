#pragma once

#include <QList>
#include <QString>

#include "bina/format/Formatter.h"
#include "bina/format/Value.h"

namespace bina::format {

// Padrão no estilo printf aplicado a um Value (escalar, array ou struct).
//
//   %[(campo[+N|-N])][flags][largura][.precisão]conversão
//
// Conversões: d i u x X o b c s f F e E g G e %%. Modificadores de tamanho (l, ll, h, z...) são aceitos e ignorados.
// Sem "(campo)", os argumentos são os valores escalares do Value em ordem (structs e arrays são achatados).
// "(campo)" seleciona um campo de struct pelo nome, com caminho por pontos ("bounds.min"), e N é somado ao valor.
//   Ex.: "%(tm_year+1900)04d-%(tm_mon+1)02d-%(tm_mday)02d"
class PrintfFormat
{
public:

    static PrintfFormat compile(const QString &spec);

    bool isValid() const { return m_error.isEmpty(); }
    QString errorString() const { return m_error; }
    QString spec() const { return m_spec; }

    // As conversões do padrão, na ordem: o campo que cada uma lê (vazio se posicional) e o tipo.
    struct Reference {
        QString key;
        QChar conversion;
    };
    QList<Reference> references() const;

    // Argumento ausente ou campo inexistente são exibidos como "<?>".
    QString format(const Value &value) const;

private:

    struct Segment {
        bool isConversion{false};
        QString literal;

        QString key;
        qint64 offset{0};
        QString flags;
        int width{-1};
        int precision{-1};
        QChar conversion;
    };

    QString render(const Segment &seg, const Value &arg) const;

    QString m_spec;
    QString m_error;
    QList<Segment> m_segments;
};

class PrintfFormatter : public Formatter
{
public:

    PrintfFormatter(const QString &id, const PrintfFormat &format);

    QString id() const override { return m_id; }
    QString displayName() const override { return m_format.spec(); }
    bool accepts(const Target &) const override { return true; }

    QString format(const Value &value) const override;

    const PrintfFormat &printf() const { return m_format; }

protected:

    QString formatScalar(const Value &value) const override { return m_format.format(value); }

private:

    QString m_id;
    PrintfFormat m_format;
};

}

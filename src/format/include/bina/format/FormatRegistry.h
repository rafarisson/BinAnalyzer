#pragma once

#include <memory>

#include <QHash>
#include <QList>
#include <QString>

#include "bina/format/Formatter.h"
#include "bina/schema/OperationResult.h"

namespace bina::format {

// Formatadores disponíveis em um documento:
//  - embutidos (dec, hex, oct, bin, float, string, bytes);
//  - padrões nomeados, definidos pelo usuário ("iso_date" -> "%(tm_year+1900)04d-...");
//  - padrões inline, referenciados diretamente pelo id "printf:<padrão>".
class FormatRegistry
{
public:

    struct Pattern {
        QString id;
        QString spec;
    };

    FormatRegistry();

    // nullptr se o formato não existe ou é um padrão inline inválido (o motivo vem de check()).
    // Os padrões inline compilados ficam em um cache limitado: consultar não muda o registro.
    const Formatter *find(const QString &id) const;
    bool contains(const QString &id) const { return find(id) != nullptr; }

    // Sucesso se o formato existe; senão diz por que (desconhecido ou padrão inline inválido).
    schema::OperationResult check(const QString &id) const;

    // id deve ser um identificador (letras, dígitos e '_') e não pode repetir um formatador existente.
    schema::OperationResult addPattern(const QString &id, const QString &spec);
    QList<Pattern> patterns() const { return m_patterns; }

    // Troca o padrão de um formato nomeado já existente. Recusa um id que não é de padrão nomeado
    // (os embutidos não mudam) e um padrão inválido; nos dois casos nada muda.
    schema::OperationResult setPattern(const QString &id, const QString &spec);

    QList<const Formatter *> formatters() const;
    QList<const Formatter *> compatible(const Target &target) const;
    bool isCompatible(const QString &id, const Target &target) const;

    // Formato usado quando o campo não define um; vazio se o campo deve apenas expandir os filhos (struct).
    QString defaultId(const Target &target) const;

    // id vazio usa defaultId(). Retorna texto vazio se o formato for desconhecido.
    QString format(const QString &id, const Target &target, const Value &value) const;

private:

    // Limite do cache de padrões inline: cada padrão digitado em um preview entraria nele.
    static constexpr qsizetype kMaxInlineCache = 256;

    std::vector<std::unique_ptr<Formatter>> m_builtin;
    std::vector<std::unique_ptr<Formatter>> m_named;
    QList<Pattern> m_patterns;
    mutable QHash<QString, std::shared_ptr<Formatter>> m_inline;
};

}

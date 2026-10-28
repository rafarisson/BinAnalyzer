#pragma once

#include <QCoreApplication>
#include <QLatin1StringView>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace bina::model {

// Roles comuns aos models editáveis. Dizem à view qual editor abrir para uma célula e com que
// opções, para o delegate escolher o editor sem conhecer as colunas do model.
enum EditorRole {
    EditorKindRole = Qt::UserRole + 100,    // "text", "combo", "spin", "readonly" ou vazio (não editável)
    ChoicesRole,                            // combo: lista de {value, text}
    MinimumRole,                            // spin
    MaximumRole                             // spin
};

namespace EditorKinds {

inline constexpr QLatin1StringView kText  ("text");
inline constexpr QLatin1StringView kCombo ("combo");
inline constexpr QLatin1StringView kSpin  ("spin");

// Texto que só se seleciona e copia: o model recusa a gravação.
inline constexpr QLatin1StringView kReadOnly ("readonly");

}

// Opções de um combo a partir dos ids. Com 'withDefault', a primeira opção é "(padrão)" e vale "".
inline QVariantList makeChoices(const QStringList &ids, bool withDefault = false)
{
    QVariantList choices;

    if (withDefault) {
        choices.append(QVariantMap{{QStringLiteral("value"), QString()},
                                   {QStringLiteral("text"), QCoreApplication::translate("Model", "(padrão)")}});
    }

    for (const QString &id : ids)
        choices.append(QVariantMap{{QStringLiteral("value"), id}, {QStringLiteral("text"), id}});

    return choices;
}

} // namespace bina::model

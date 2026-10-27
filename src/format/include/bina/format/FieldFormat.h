#pragma once

#include <QString>

#include "bina/format/Formatter.h"

namespace bina::schema { class Field; class TypeRegistry; }

namespace bina::format {

class FormatRegistry;

// Campos auto_pad existem só para alinhar a estrutura: não são exibidos nem formatados.
bool isDisplayable(const schema::Field &field);

// Tipo e quantidade do campo, para escolher formatadores compatíveis.
Target targetFor(const schema::Field &field, const schema::TypeRegistry &types);

// Um formato explícito só serve se o formatador existe e aceita o tipo e o count do campo.
// Vazio (formato padrão) é sempre válido. É a regra única usada ao editar e ao carregar.
bool isFormatValid(const schema::Field &field, const schema::TypeRegistry &types,
                   const FormatRegistry &formats, const QString &formatId);

// Formato efetivo de um campo exibível: o do campo, senão o padrão do tipo struct,
// senão o padrão do tipo primitivo. Vazio se não há formato (auto_pad, ou struct apenas expandida).
QString effectiveFormatId(const schema::Field &field, const schema::TypeRegistry &types, const FormatRegistry &formats);

}

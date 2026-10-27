#pragma once

#include <QString>

namespace bina::schema {

// Normaliza um identificador (id de tipo ou nome de campo): letras e dígitos de qualquer idioma
// (acentos incluídos) e "_"; o resto vira "_". Espaços nas pontas são removidos.
QString normalizeIdentifier(const QString &id);

} // namespace bina::schema

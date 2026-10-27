#pragma once

#include <QString>

namespace bina::schema {

// Ids dos tipos que todo registro tem (ver TypeRegistry::withBuiltins()).
namespace PrimitiveTypes {

inline constexpr QLatin1StringView kChar        ("char");
inline constexpr QLatin1StringView kInt8        ("int8");
inline constexpr QLatin1StringView kUInt8       ("uint8");
inline constexpr QLatin1StringView kInt16       ("int16");
inline constexpr QLatin1StringView kUInt16      ("uint16");
inline constexpr QLatin1StringView kInt32       ("int32");
inline constexpr QLatin1StringView kUInt32      ("uint32");
inline constexpr QLatin1StringView kEnum        ("enum");
inline constexpr QLatin1StringView kFloat       ("float");
// Ponteiro de 32 bits (4 bytes sem interpretação numérica), como "void *" em um alvo de 32 bits.
inline constexpr QLatin1StringView kVoid        ("void");
inline constexpr QLatin1StringView kInt64       ("int64");
inline constexpr QLatin1StringView kUInt64      ("uint64");
inline constexpr QLatin1StringView kDouble      ("double");

// Tipo implícito de campos que possuem filhos: o layout vem dos filhos, não do tipo.
inline constexpr QLatin1StringView kStruct      ("struct");

} // namespace PrimitiveTypes

// Nomes que a aplicação dá aos campos que ela cria.
namespace Names {

inline constexpr QLatin1StringView kAutoPad     ("auto_pad");
inline constexpr QLatin1StringView kNewField    ("field");
inline constexpr QLatin1StringView kNewGroup    ("group");
inline constexpr QLatin1StringView kFallback    ("new_element");

} // namespace Names

} // namespace bina::schema

#pragma once

#include <memory>
#include <vector>

#include "bina/format/Formatter.h"

namespace bina::format {

namespace FormatIds {

inline constexpr QLatin1StringView kDecimal ("dec");
inline constexpr QLatin1StringView kHex     ("hex");
inline constexpr QLatin1StringView kOctal   ("oct");
inline constexpr QLatin1StringView kBinary  ("bin");
inline constexpr QLatin1StringView kFloat   ("float");
inline constexpr QLatin1StringView kString  ("string");
inline constexpr QLatin1StringView kBytes   ("bytes");

// Prefixo de padrões inline: "printf:%02d:%02d".
inline constexpr QLatin1StringView kPrintfPrefix ("printf:");

}

// dec, hex, oct, bin, float, string e bytes.
std::vector<std::unique_ptr<Formatter>> createBuiltinFormatters();

// Escapa caracteres não imprimíveis (\n, \t, \xNN, ...).
QString escapeText(const QString &text);

}

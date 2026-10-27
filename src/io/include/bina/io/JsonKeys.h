#pragma once

#include <QString>

namespace bina::io {

namespace JsonKeys {

inline constexpr QLatin1StringView kVersion     ("version");
inline constexpr QLatin1StringView kMaxAlign    ("maxAlign");
inline constexpr QLatin1StringView kByteOrder   ("byteOrder");
inline constexpr QLatin1StringView kFormatters  ("formatters");
inline constexpr QLatin1StringView kPattern     ("pattern");
inline constexpr QLatin1StringView kTypes       ("types");
inline constexpr QLatin1StringView kElements    ("elements");
inline constexpr QLatin1StringView kId          ("id");
inline constexpr QLatin1StringView kName        ("name");
inline constexpr QLatin1StringView kType        ("type");
inline constexpr QLatin1StringView kCount       ("count");
inline constexpr QLatin1StringView kFormatter   ("formatter");

}

} // namespace bina::io

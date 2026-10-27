#pragma once

#include <QCoreApplication>
#include <QString>

#include "bina/schema/OperationResult.h"

namespace bina::schema {

// Como os bytes de um tipo primitivo devem ser interpretados.
enum class Representation {
    None,       // não é primitivo (struct/group)
    Signed,
    Unsigned,
    Float,
    Char,
    Enum,
    Opaque      // bytes sem interpretação numérica
};

// Regras do alinhamento: o de cada tipo e o máximo que o documento aplica aos campos.
namespace Alignment {

inline constexpr int kDefaultMax = 4;

constexpr bool isPowerOfTwo(int value) noexcept
{
    return value > 0 && (value & (value - 1)) == 0;
}

// O alinhamento máximo dos campos precisa ser uma potência de 2.
inline OperationResult validateMax(int align)
{
    return isPowerOfTwo(align) ? OperationResult::success()
                               : OperationResult::failure(QCoreApplication::translate("bina", "O alinhamento máximo deve ser uma potência de 2"));
}

} // namespace Alignment

} // namespace bina::schema

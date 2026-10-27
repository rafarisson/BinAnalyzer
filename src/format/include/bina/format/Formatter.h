#pragma once

#include <QString>

#include "bina/schema/Types.h"
#include "bina/format/Value.h"

namespace bina::format {

// Descreve o campo a ser exibido, para decidir quais formatadores são compatíveis.
struct Target
{
    Representation representation{Representation::None};
    int count{1};
    bool isStruct{false};
};

class Formatter
{
public:

    virtual ~Formatter() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual bool accepts(const Target &target) const = 0;

    // Por padrão, arrays, textos e buffers são formatados elemento a elemento
    // com formatScalar(); formatadores que tratam o valor inteiro sobrescrevem este método.
    virtual QString format(const Value &value) const;

protected:

    virtual QString formatScalar(const Value &value) const = 0;
};

}

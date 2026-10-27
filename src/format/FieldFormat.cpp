#include "bina/format/FieldFormat.h"
#include "bina/format/FormatRegistry.h"

// bina::schema
#include "bina/schema/DataType.h"
#include "bina/schema/Field.h"
#include "bina/schema/LayoutCalculator.h"
#include "bina/schema/Builtins.h"
#include "bina/schema/TypeRegistry.h"

namespace bina::format {

bool isDisplayable(const schema::Field &field)
{
    return !field.isAutoPad();
}

Target targetFor(const schema::Field &field, const schema::TypeRegistry &types)
{
    Target target;
    target.count = field.count();

    if (field.isStruct()) {
        target.isStruct = true;
        return target;
    }

    if (const schema::DataType *type = types.find(field.typeId())) {
        target.isStruct = type->definition() != nullptr;
        target.representation = type->representation();
    }

    return target;
}

bool isFormatValid(const schema::Field &field, const schema::TypeRegistry &types,
                   const FormatRegistry &formats, const QString &formatId)
{
    return formatId.isEmpty() || formats.isCompatible(formatId, targetFor(field, types));
}

QString effectiveFormatId(const schema::Field &field, const schema::TypeRegistry &types, const FormatRegistry &formats)
{
    if (!isDisplayable(field))
        return {};

    if (!field.formatId().isEmpty())
        return field.formatId();

    if (const schema::DataType *type = types.find(field.typeId()); type && type->definition())
        return type->formatId();

    return formats.defaultId(targetFor(field, types));
}

}

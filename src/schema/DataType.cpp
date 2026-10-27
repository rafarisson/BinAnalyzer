#include "bina/schema/DataType.h"

namespace bina::schema {

std::unique_ptr<DataType> DataType::makePrimitive(const QString &id, int size, int align, Representation representation)
{
    return std::unique_ptr<DataType>(new DataType(id, PrimitiveInfo{size, align, representation}));
}

std::unique_ptr<DataType> DataType::makeStruct(const QString &id, std::unique_ptr<Field> definition, const QString &formatId)
{
    return std::unique_ptr<DataType>(new DataType(id, StructInfo{std::move(definition), formatId}));
}

int DataType::size() const
{
    const auto *primitive = std::get_if<PrimitiveInfo>(&m_info);
    return primitive ? primitive->size : 0;
}

int DataType::align() const
{
    const auto *primitive = std::get_if<PrimitiveInfo>(&m_info);
    return primitive ? primitive->align : 1;
}

Representation DataType::representation() const
{
    const auto *primitive = std::get_if<PrimitiveInfo>(&m_info);
    return primitive ? primitive->representation : Representation::None;
}

QString DataType::formatId() const
{
    const auto *info = std::get_if<StructInfo>(&m_info);
    return info ? info->formatId : QString();
}

const Field *DataType::definition() const
{
    const auto *info = std::get_if<StructInfo>(&m_info);
    return info ? info->definition.get() : nullptr;
}

} // namespace bina::schema

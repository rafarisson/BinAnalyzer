#include "bina/document/TypeCatalog.h"

#include "bina/schema/Identifier.h"

namespace bina::document {

void TypeCatalog::add(const DocumentDto &data)
{
    for (DocumentFormatterDto formatter : data.formatters) {
        formatter.id = formatter.id.trimmed();
        m_formatters.insert(formatter.id, formatter);
    }

    for (DocumentTypeDto type : data.types) {
        type.id = schema::normalizeIdentifier(type.id);

        if (!m_types.contains(type.id))
            m_typeOrder << type.id;

        m_types.insert(type.id, type);
    }
}

void TypeCatalog::clear()
{
    m_types.clear();
    m_typeOrder.clear();
    m_formatters.clear();
}

const DocumentTypeDto *TypeCatalog::type(const QString &id) const
{
    const auto it = m_types.constFind(id);
    return it != m_types.cend() ? &it.value() : nullptr;
}

const DocumentFormatterDto *TypeCatalog::formatter(const QString &id) const
{
    const auto it = m_formatters.constFind(id);
    return it != m_formatters.cend() ? &it.value() : nullptr;
}

} // namespace bina::document

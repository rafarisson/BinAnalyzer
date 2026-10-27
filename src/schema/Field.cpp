#include <atomic>

#include "bina/schema/Field.h"
#include "bina/schema/Identifier.h"
#include "bina/schema/Builtins.h"

namespace bina::schema {

namespace {

Field::Id nextId()
{
    static std::atomic<Field::Id> counter{0};
    return ++counter;
}

}

Field::Field(const QString &name)
    : m_id{nextId()}
    , m_typeId{PrimitiveTypes::kUInt32}
{
    if (setName(name) != Edit::Changed)
        m_name = Names::kFallback;
}

std::unique_ptr<Field> Field::makeAutoPad(int size)
{
    auto pad = std::make_unique<Field>();

    pad->m_autoPad = true;
    pad->m_name = Names::kAutoPad;
    pad->m_typeId = PrimitiveTypes::kUInt8;
    pad->m_count = qMax(size, 1);

    return pad;
}

Field::~Field() = default;

Field::Edit Field::setName(const QString &name)
{
    const QString normalized = normalizeIdentifier(name);
    if (normalized.isEmpty())
        return Edit::Invalid;
    if (m_name == normalized)
        return Edit::Unchanged;

    m_name = normalized;
    return Edit::Changed;
}

Field::Edit Field::setTypeId(const QString &id)
{
    const QString normalized = normalizeIdentifier(id);
    if (normalized.isEmpty())
        return Edit::Invalid;
    if (m_typeId == normalized)
        return Edit::Unchanged;

    m_typeId = normalized;
    return Edit::Changed;
}

Field::Edit Field::setCount(int count)
{
    if (count <= 0 || count > kMaxCount)
        return Edit::Invalid;
    if (m_count == count)
        return Edit::Unchanged;

    m_count = count;
    return Edit::Changed;
}

Field::Edit Field::setFormatId(const QString &id)
{
    // Não é um identificador: pode ser um padrão "printf:..."; vazio volta ao formato padrão.
    const QString trimmed = id.trimmed();
    if (m_formatId == trimmed)
        return Edit::Unchanged;

    m_formatId = trimmed;
    return Edit::Changed;
}

bool Field::isStruct() const
{
    return m_typeId == PrimitiveTypes::kStruct || !m_children.empty();
}

int Field::row() const
{
    if (!m_parent)
        return -1;

    for (int i = 0; i < m_parent->childCount(); ++i) {
        if (m_parent->m_children[i].get() == this)
            return i;
    }

    return -1;
}

Field *Field::child(int row) const
{
    if (row < 0 || row >= childCount())
        return nullptr;

    return m_children[row].get();
}

std::unique_ptr<Field> Field::clone(bool withAutoPads) const
{
    auto copy = std::make_unique<Field>();

    // Os valores já foram validados: copia direto, sem passar por setName() e companhia.
    copy->m_name = m_name;
    copy->m_typeId = m_typeId;
    copy->m_count = m_count;
    copy->m_formatId = m_formatId;
    copy->m_autoPad = m_autoPad;

    for (const auto &child : m_children) {
        if (withAutoPads || !child->m_autoPad)
            copy->insertExistingChild(copy->childCount(), child->clone(withAutoPads));
    }

    return copy;
}

Field *Field::insertChild(int position, const QString &name)
{
    auto item = std::make_unique<Field>(name);
    Field *raw = item.get();

    if (!insertExistingChild(position, std::move(item)))
        return nullptr;

    return raw;
}

bool Field::insertExistingChild(int position, std::unique_ptr<Field> &&field)
{
    if (!field || field->m_parent || position < 0 || position > childCount())
        return false;

    for (const Field *p = this; p; p = p->m_parent) {
        if (p == field.get())
            return false;
    }

    field->m_parent = this;
    m_children.insert(m_children.begin() + position, std::move(field));

    return true;
}

std::unique_ptr<Field> Field::takeChild(int position)
{
    if (position < 0 || position >= childCount())
        return nullptr;

    std::unique_ptr<Field> item = std::move(m_children[position]);
    m_children.erase(m_children.begin() + position);
    item->m_parent = nullptr;

    return item;
}

bool Field::removeChild(int position)
{
    return takeChild(position) != nullptr;
}

} // namespace bina::schema

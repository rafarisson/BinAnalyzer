#include <limits>

#include "bina/schema/LayoutCalculator.h"
#include "bina/schema/Builtins.h"
#include "bina/schema/DataType.h"
#include "bina/schema/Field.h"
#include "bina/schema/TypeRegistry.h"
#include "bina/schema/Types.h"

namespace bina::schema {

LayoutCalculator::LayoutCalculator(const TypeRegistry *registry, int maxAlign)
    : m_registry{registry}
    , m_maxAlign{Alignment::isPowerOfTwo(maxAlign) ? maxAlign : 1}
{}

LayoutCalculator::Layout LayoutCalculator::layoutOf(const Field *field) const
{
    if (!field || !m_registry)
        return {};

    if (field->isStruct())
        return structLayout(field);

    const DataType *type = m_registry->find(field->typeId());
    if (!type)
        return {};

    // Um tipo struct customizado tem o layout dos campos da sua definição.
    if (type->category() == DataType::Struct) {
        if (const auto it = m_typeLayouts.constFind(type->id()); it != m_typeLayouts.constEnd())
            return *it;

        const Layout layout = structLayout(type->definition());
        m_typeLayouts.insert(type->id(), layout);
        return layout;
    }

    return {type->size(), qMin(type->align(), m_maxAlign)};
}

int LayoutCalculator::sizeOf(const Field *field) const
{
    if (!field)
        return 0;

    const qint64 size = qint64(layoutOf(field).size) * field->count();
    return size > std::numeric_limits<int>::max() ? 0 : int(size);
}

// Layout no estilo C: cada campo é alinhado ao seu alinhamento (limitado por maxAlign) e o
// tamanho final é arredondado ao maior alinhamento. Retorna size == 0 se o layout for inválido.
LayoutCalculator::Layout LayoutCalculator::structLayout(const Field *field, QList<PadSlot> *plan) const
{
    if (plan)
        plan->clear();

    if (!field || !m_registry)
        return {};

    Layout layout;
    qint64 offset = 0;
    int fields = 0;

    auto padFor = [&offset](int align) { return (align - offset % align) % align; };

    for (int i = 0; i < field->childCount(); ++i) {
        const Field *child = field->child(i);
        if (child->isAutoPad())
            continue;

        const int fieldSize = sizeOf(child);
        if (fieldSize <= 0)
            return {};

        const int align = layoutOf(child).align;
        const qint64 pad = padFor(align);

        if (pad > 0 && plan)
            plan->append({i, int(pad)});

        offset += pad + fieldSize;
        layout.align = qMax(layout.align, align);
        ++fields;
    }

    if (fields == 0)
        return {};

    const qint64 tail = padFor(layout.align);
    if (tail > 0 && plan)
        plan->append({field->childCount(), int(tail)});

    offset += tail;
    if (offset > std::numeric_limits<int>::max())
        return {};

    layout.size = int(offset);

    return layout;
}

QList<LayoutCalculator::PadSlot> LayoutCalculator::padPlan(const Field *field) const
{
    QList<PadSlot> plan;
    structLayout(field, &plan);
    return plan;
}

QList<int> LayoutCalculator::paddings(const Field *field) const
{
    QList<PadSlot> plan;
    if (!field || structLayout(field, &plan).size <= 0)
        return {};

    // PadSlot::beforeIndex conta os auto_pad entre os filhos; aqui só os campos contam.
    QList<int> ordinalOfChild;
    int fields = 0;
    for (int i = 0; i < field->childCount(); ++i) {
        ordinalOfChild.append(fields);
        if (!field->child(i)->isAutoPad())
            ++fields;
    }

    QList<int> out(fields + 1, 0);
    for (const PadSlot &slot : std::as_const(plan)) {
        const int ordinal = slot.beforeIndex >= field->childCount() ? fields : ordinalOfChild.at(slot.beforeIndex);
        out[ordinal] = slot.size;
    }

    return out;
}

} // namespace bina::schema

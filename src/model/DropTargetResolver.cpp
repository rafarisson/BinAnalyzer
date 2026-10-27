#include "bina/model/DropTargetResolver.h"

// bina::schema
#include "bina/schema/Field.h"

namespace bina::model {

std::optional<DropTarget> resolveDropTarget(schema::Field *parent, int row)
{
    using document::FieldTree;

    if (!parent)
        return std::nullopt;

    if (row < 0)
        return DropTarget{parent, FieldTree::Onto};

    const int count = parent->childCount();
    int i = qMin(row, count);

    // Um auto_pad não é um ponto de soltura: vale o campo seguinte.
    while (i < count && parent->child(i)->isAutoPad())
        ++i;

    if (i < count)
        return DropTarget{parent->child(i), FieldTree::Before};

    for (int j = count - 1; j >= 0; --j) {
        if (!parent->child(j)->isAutoPad())
            return DropTarget{parent->child(j), FieldTree::After};
    }

    // Pai sem campos: soltar nele.
    return DropTarget{parent, FieldTree::Onto};
}

} // namespace bina::model

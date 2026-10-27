// Inserir, remover, mover, soltar e agrupar campos (parte de FieldTree).

#include "bina/document/FieldTree.h"

// bina::schema
#include "bina/schema/LayoutCalculator.h"
#include "bina/schema/Builtins.h"

namespace bina::document {

bool FieldTree::insert(schema::Field *parent, int position, int count)
{
    if (!parent || count <= 0)
        return false;

    // Cada campo entra logo depois do anterior: os auto_pad criados no caminho deslocam as linhas.
    schema::Field *previous = nullptr;

    for (int i = 0; i < count; ++i) {
        schema::Field *created = insertField(parent, previous ? previous->row() + 1 : position);
        if (!created)
            return i > 0;

        previous = created;
    }

    return true;
}

schema::Field *FieldTree::insertField(schema::Field *parent, int row)
{
    if (!parent || parent->isAutoPad())
        return nullptr;

    row = qBound(0, row, calculateInsertPosition(parent));

    touch();

    emit aboutToInsert(parent, row, row);
    schema::Field *field = parent->insertChild(row, uniqueFieldName(parent, schema::Names::kNewField));
    emit inserted();

    becomeStruct(parent);
    updatePadding(parent);

    return field;
}

bool FieldTree::remove(schema::Field *parent, int position, int count)
{
    if (!parent || count <= 0 || position < 0 || position + count > parent->childCount())
        return false;

    for (int i = 0; i < count; ++i)
        if (parent->child(position + i)->isAutoPad())
            return false;

    touch();

    emit aboutToRemove(parent, position, position + count - 1);
    for (int i = 0; i < count; ++i)
        parent->removeChild(position);
    emit removed();

    updatePadding(parent);

    return true;
}

bool FieldTree::move(schema::Field *srcParent, int srcRow, schema::Field *dstParent, int dstRow)
{
    if (srcParent == dstParent && (dstRow == srcRow || dstRow == srcRow + 1))
        return false;

    const int finalRow = calculateDestinationRowForMove(srcParent, srcRow, dstParent, dstRow);
    if (finalRow < 0)
        return false;

    // A linha informada às views é a anterior à remoção (convenção do beginMoveRows).
    if (dstRow < 0)
        dstRow = calculateInsertPosition(dstParent);

    touch();

    emit aboutToMove(srcParent, srcRow, dstParent, dstRow);

    std::unique_ptr<schema::Field> taken = srcParent->takeChild(srcRow);
    bool ok = taken != nullptr;
    if (ok && !dstParent->insertExistingChild(finalRow, std::move(taken))) {
        // 'taken' não foi consumido: volta ao lugar de origem.
        srcParent->insertExistingChild(srcRow, std::move(taken));
        ok = false;
    }

    emit moved();

    if (!ok)
        return false;

    becomeStruct(dstParent);

    updatePadding(srcParent);
    if (dstParent != srcParent)
        updatePadding(dstParent);

    return true;
}

schema::Field *FieldTree::addAfter(schema::Field *field)
{
    if (!field || field->isAutoPad() || !field->parent())
        return nullptr;

    return insertField(field->parent(), field->row() + 1);
}

schema::Field *FieldTree::addAfterParent(schema::Field *field)
{
    schema::Field *parent = field && !field->isAutoPad() ? field->parent() : nullptr;
    if (!parent || !parent->parent())
        return nullptr;

    return insertField(parent->parent(), parent->row() + 1);
}

schema::Field *FieldTree::addInside(schema::Field *field)
{
    if (!field || field->isAutoPad())
        return nullptr;

    return insertField(field, field->childCount());
}

bool FieldTree::removeField(schema::Field *field)
{
    if (!field || field->isAutoPad() || !field->parent())
        return false;

    return remove(field->parent(), field->row(), 1);
}

bool FieldTree::canDrop(const schema::Field *source, const schema::Field *target, DropZone zone) const
{
    using schema::LayoutCalculator;

    if (!source || !target || source == target || !source->parent())
        return false;

    if (source->isAutoPad() || target->isAutoPad())
        return false;

    // Um campo não pode ir para dentro de si mesmo.
    for (const schema::Field *p = target; p; p = p->parent())
        if (p == source)
            return false;

    if (zone == Onto)
        return true;

    // Antes/depois exige um pai e uma posição diferente da atual.
    const schema::Field *dstParent = target->parent();
    if (!dstParent)
        return false;

    const int dstRow = target->row() + (zone == After ? 1 : 0);

    return !(dstParent == source->parent() && (dstRow == source->row() || dstRow == source->row() + 1));
}

bool FieldTree::drop(schema::Field *source, schema::Field *target, DropZone zone)
{
    if (!canDrop(source, target, zone))
        return false;

    if (zone == Onto) {
        bool hasFields = false;
        for (int i = 0; i < target->childCount() && !hasFields; ++i)
            hasFields = !target->child(i)->isAutoPad();

        // Struct existente recebe o campo no fim; um campo comum forma uma nova struct com ele.
        return hasFields
                   ? move(source->parent(), source->row(), target, calculateInsertPosition(target))
                   : groupWith(target, source);
    }

    return move(source->parent(), source->row(), target->parent(), target->row() + (zone == After ? 1 : 0));
}

// Cria uma struct na posição de 'target' e põe 'target' e 'source' dentro dela.
bool FieldTree::groupWith(schema::Field *target, schema::Field *source)
{
    schema::Field *parent = target->parent();
    if (!parent)
        return false;

    schema::Field *group = insertField(parent, target->row());
    if (!group)
        return false;

    group->setName(uniqueFieldName(parent, schema::Names::kNewGroup));

    // As linhas são recalculadas a cada passo: os auto_pad podem deslocar os campos.
    if (!move(parent, target->row(), group, 0)
        || !move(source->parent(), source->row(), group, calculateInsertPosition(group)))
        return false;

    emit fieldChanged(group);

    return true;
}

QString FieldTree::uniqueFieldName(const schema::Field *parent, const QString &base) const
{
    QStringList used;
    for (int i = 0; i < parent->childCount(); ++i)
        used << parent->child(i)->name();

    for (int n = 1;; ++n) {
        const QString name = QStringLiteral("%1_%2").arg(base).arg(n);
        if (!used.contains(name))
            return name;
    }
}

int FieldTree::calculateInsertPosition(const schema::Field *parent) const
{
    if (!parent)
        return 0;

    // Só o auto_pad final é reservado; os demais ficam entre os campos e são recalculados.
    const int last = parent->childCount() - 1;

    return last >= 0 && parent->child(last)->isAutoPad() ? last : parent->childCount();
}

int FieldTree::calculateDestinationRowForMove(const schema::Field *srcParent, int srcRow, schema::Field *dstParent, int dstRow) const
{
    using schema::LayoutCalculator;

    if (dstRow < 0)
        dstRow = calculateInsertPosition(dstParent);

    if (!srcParent || !dstParent || dstParent->isAutoPad())
        return -1;

    if (srcRow < 0 || srcRow >= srcParent->childCount())
        return -1;

    const schema::Field *item = srcParent->child(srcRow);

    if (!item || item->isAutoPad() || item == dstParent)
        return -1;

    for (const schema::Field *p = dstParent; p; p = p->parent()) {
        if (p == item)
            return -1;
    }

    // Não pode mover para depois do auto_pad final.
    int maxRow = dstParent->childCount();
    const int last = maxRow - 1;
    if (last >= 0 && dstParent->child(last)->isAutoPad())
        maxRow = last;

    if (dstRow > maxRow)
        return -1;

    // Converte a posição do beginMoveRows() (antes da remoção) para a posição final.
    int finalRow = dstRow;

    // No mesmo pai, tirar o item de cima desloca as linhas seguintes.
    if (srcParent == dstParent && srcRow < finalRow && finalRow > 0)
        finalRow--;

    return qMax(finalRow, 0);
}

} // namespace bina::document

// Protocolo de mime para arrastar e soltar (parte de SchemaTreeModel).

#include "bina/model/SchemaTreeModel.h"
#include "bina/model/DropTargetResolver.h"
#include "bina/model/FieldMimeCodec.h"

// bina::schema
#include "bina/model/FieldRef.h"
#include "bina/schema/Field.h"
#include "bina/document/FieldTree.h"
#include "bina/schema/LayoutCalculator.h"

namespace bina::model {

Qt::DropActions SchemaTreeModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions SchemaTreeModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList SchemaTreeModel::mimeTypes() const
{
    return FieldMimeCodec::mimeTypes();
}

schema::Field *SchemaTreeModel::sourceFromMime(const QMimeData *data) const
{
    FieldRef ref;

    if (!m_tree || !FieldMimeCodec::decode(data, ref))
        return nullptr;

    // O arrasto precisa ter nascido neste model e na estrutura atual.
    if (ref.revision != m_tree->revision())
        return nullptr;

    return FieldRef::toNode(ref, m_tree->root(), m_token);
}

bool SchemaTreeModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(column);

    if (action != Qt::MoveAction)
        return false;

    schema::Field *source = sourceFromMime(data);
    if (!source)
        return false;

    const auto drop = resolveDropTarget(itemFromIndex(parent), row);

    return drop && m_tree->canDrop(source, drop->target, drop->zone);
}

QMimeData *SchemaTreeModel::mimeData(const QModelIndexList &indexes) const
{
    const schema::Field *item = nullptr;

    for (const QModelIndex &index : indexes) {
        if (!index.isValid())
            continue;

        const schema::Field *field = itemFromIndex(index);

        // Um campo por arrasto: depois de um MoveAction a view remove tudo o que estava selecionado,
        // e só um campo é movido.
        if (item && field != item)
            return nullptr;

        item = field;
    }

    if (!m_tree || !item || item == m_tree->root() || item->isAutoPad())
        return nullptr;

    const auto ref = FieldRef::fromNode(item, m_tree->root(), m_token, m_tree->revision());

    return ref.isValid() ? FieldMimeCodec::encode(ref) : nullptr;
}

bool SchemaTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column);

    if (action != Qt::MoveAction)
        return false;

    schema::Field *source = sourceFromMime(data);
    if (!source)
        return false;

    const auto drop = resolveDropTarget(itemFromIndex(parent), row);

    if (!drop || !m_tree->drop(source, drop->target, drop->zone))
        return false;

    // O campo foi movido (a identidade é preservada). Devolvemos true: o drop foi aceito. A view
    // ainda vai remover a origem; esse pedido será reconhecido e ignorado em removeRows().
    m_pendingMove.arm(source);

    emit revealRequested(indexFromItem(source));

    return true;
}

} // namespace bina::model

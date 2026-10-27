// Ações de menu, inserção/remoção/movimento de linhas e soltura por índices (parte de
// SchemaTreeModel).

#include "bina/model/SchemaTreeModel.h"

// bina::schema
#include "bina/schema/Field.h"
#include "bina/document/FieldTree.h"
#include "bina/schema/LayoutCalculator.h"

namespace bina::model {

QVariantList SchemaTreeModel::actions(const QModelIndex &index) const
{
    const schema::Field *field = m_tree ? itemFromIndex(index) : nullptr;
    if (!field || field->isAutoPad())
        return {};

    const bool hasParent = field->parent() != nullptr;
    const bool hasGrandparent = hasParent && field->parent()->parent() != nullptr;

    auto action = [](const QString &id, const QString &text, bool enabled) {
        return QVariant(QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("text"), text},
                                    {QStringLiteral("enabled"), enabled}});
    };

    QVariantList list{
        action(QStringLiteral("insertBelow"), tr("Inserir campo abaixo"), hasParent),
        action(QStringLiteral("insertBelowParent"), tr("Inserir campo abaixo do pai"), hasGrandparent),
        action(QStringLiteral("insertInside"), tr("Inserir campo dentro"), true),
        action(QStringLiteral("remove"), tr("Remover campo"), hasParent)
    };

    // Só onde se aplicam: a árvore de um tipo, por exemplo, não cria tipos.
    if (m_typeExtraction && m_tree->canCollapseToType(field)) {
        QVariantMap extract = action(QStringLiteral("extractType"), tr("Criar tipo a partir do grupo..."), true).toMap();
        extract.insert(QStringLiteral("prompt"), tr("Novo tipo a partir de '%1'").arg(field->name()));
        list << QVariant(extract);
    }

    if (m_tree->canInlineType(field))
        list << action(QStringLiteral("inlineType"), tr("Converter tipo '%1' em grupo").arg(field->typeId()), true);

    return list;
}

bool SchemaTreeModel::trigger(const QModelIndex &index, const QString &actionId, const QString &text)
{
    schema::Field *field = m_tree ? itemFromIndex(index) : nullptr;
    if (!field)
        return false;

    schema::Field *created = nullptr;
    bool ok = false;

    if (actionId == QLatin1String("insertBelow"))
        ok = (created = m_tree->addAfter(field)) != nullptr;
    else if (actionId == QLatin1String("insertBelowParent"))
        ok = (created = m_tree->addAfterParent(field)) != nullptr;
    else if (actionId == QLatin1String("insertInside"))
        ok = (created = m_tree->addInside(field)) != nullptr;
    else if (actionId == QLatin1String("remove"))
        ok = m_tree->removeField(field);
    else if (actionId == QLatin1String("extractType") && m_typeExtraction && m_tree->canCollapseToType(field)) {
        emit extractTypeRequested(field, text);
        ok = true;
    }
    else if (actionId == QLatin1String("inlineType"))
        ok = m_tree->inlineType(field);

    if (!ok)
        return false;

    if (created)
        emit revealRequested(indexFromItem(created));

    return true;
}

bool SchemaTreeModel::canDropAt(const QModelIndex &source, const QModelIndex &target, DropZone zone) const
{
    return m_tree && source.isValid() && target.isValid()
           && m_tree->canDrop(itemFromIndex(source), itemFromIndex(target), document::FieldTree::DropZone(zone));
}

bool SchemaTreeModel::dropAt(const QModelIndex &source, const QModelIndex &target, DropZone zone)
{
    if (!canDropAt(source, target, zone))
        return false;

    schema::Field *moved = itemFromIndex(source);

    if (!m_tree->drop(moved, itemFromIndex(target), document::FieldTree::DropZone(zone)))
        return false;

    emit revealRequested(indexFromItem(moved));

    return true;
}

bool SchemaTreeModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    if (!m_tree)
        return false;

    return m_tree->insert(itemFromIndex(parent), position, rows);
}

bool SchemaTreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    if (!m_tree)
        return false;

    schema::Field *field = itemFromIndex(parent);

    // A origem de um MoveAction já foi movida por dropMimeData(); a view só está limpando.
    if (rows == 1 && field && m_pendingMove.take(field->child(position)))
        return true;

    return m_tree->remove(field, position, rows);
}

bool SchemaTreeModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    if (!m_tree || count != 1)
        return false;

    return m_tree->move(itemFromIndex(sourceParent), sourceRow, itemFromIndex(destinationParent), destinationChild);
}

} // namespace bina::model

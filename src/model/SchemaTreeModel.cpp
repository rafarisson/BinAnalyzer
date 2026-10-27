// Estrutura, dados, flags e roles do model da árvore de campos.

#include <QRandomGenerator>

#include "bina/model/SchemaTreeModel.h"
#include "bina/model/EditorRoles.h"
#include "bina/model/SchemaColumns.h"

// bina::schema
#include "bina/schema/Field.h"
#include "bina/document/FieldTree.h"
#include "bina/schema/LayoutCalculator.h"

namespace bina::model {

static_assert(int(SchemaTreeModel::Before) == int(document::FieldTree::Before)
              && int(SchemaTreeModel::Onto) == int(document::FieldTree::Onto)
              && int(SchemaTreeModel::After) == int(document::FieldTree::After));

SchemaTreeModel::SchemaTreeModel(QObject *parent)
    : QAbstractItemModel{parent}
{
    // Nunca 0 (inválido); diferencia este model de qualquer outro que gere o mesmo tipo de mime.
    m_token = QRandomGenerator::global()->generate64() | 1;
}

SchemaTreeModel::SchemaTreeModel(document::FieldTree *model, QObject *parent)
    : SchemaTreeModel{parent}
{
    setModel(model);
}

void SchemaTreeModel::setModel(document::FieldTree *model)
{
    if (m_tree == model)
        return;

    beginResetModel();

    if (m_tree)
        disconnect(m_tree, nullptr, this, nullptr);

    m_tree = model;
    m_pendingMove.reset();

    if (m_tree) {
        connect(m_tree, &document::FieldTree::aboutToReset, this, &SchemaTreeModel::beginResetModel);
        connect(m_tree, &document::FieldTree::reset, this, &SchemaTreeModel::endResetModel);

        connect(m_tree, &document::FieldTree::aboutToInsert, this, [this](const schema::Field *parent, int first, int last) {
            beginInsertRows(indexFromItem(parent), first, last);
        });
        connect(m_tree, &document::FieldTree::inserted, this, &SchemaTreeModel::endInsertRows);

        connect(m_tree, &document::FieldTree::aboutToRemove, this, [this](const schema::Field *parent, int first, int last) {
            beginRemoveRows(indexFromItem(parent), first, last);
        });
        connect(m_tree, &document::FieldTree::removed, this, &SchemaTreeModel::endRemoveRows);

        connect(m_tree, &document::FieldTree::aboutToMove, this, [this](const schema::Field *srcParent, int srcRow, const schema::Field *dstParent, int dstRow) {
            beginMoveRows(indexFromItem(srcParent), srcRow, srcRow, indexFromItem(dstParent), dstRow);
        });
        connect(m_tree, &document::FieldTree::moved, this, &SchemaTreeModel::endMoveRows);

        connect(m_tree, &document::FieldTree::fieldChanged, this, [this](const schema::Field *field) {
            if (!field)
                return;
            onFieldChanged(field->parent());
            onFieldChanged(field);
        });

        connect(m_tree, &document::FieldTree::editRejected, this, &SchemaTreeModel::errorOccurred);
    }

    endResetModel();
}

const schema::Field *SchemaTreeModel::root() const
{
    return m_tree ? m_tree->root() : nullptr;
}

QModelIndex SchemaTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column < 0 || column >= ColumnCount)
        return {};

    auto *parentItem = itemFromIndex(parent);
    if (!parentItem)
        return {};

    auto *child = parentItem->child(row);
    if (!child)
        return {};

    return createIndex(row, column, child);
}

QModelIndex SchemaTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return {};

    auto *item = itemFromIndex(child);
    if (!item || item == root())
        return {};

    auto *parentItem = item->parent();

    if (!parentItem || parentItem == root())
        return {};

    return createIndex(parentItem->row(), 0, parentItem);
}

int SchemaTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    auto *parentItem = itemFromIndex(parent);

    return parentItem ? parentItem->childCount() : 0;
}

int SchemaTreeModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

bool SchemaTreeModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return false;

    auto *item = itemFromIndex(parent);

    return item && item->childCount() > 0;
}

QVariant SchemaTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Vertical)
        return section;

    const SchemaColumn *column = SchemaColumns::at(section);

    return column ? QVariant(tr(column->header)) : QVariant();
}

// Que editor a célula pede (vazio se não é editável).
QString SchemaTreeModel::editorKind(const QModelIndex &index) const
{
    if (!(flags(index) & Qt::ItemIsEditable))
        return {};

    const SchemaColumn *column = SchemaColumns::at(index.column());

    return column ? QString(column->editor) : QString();
}

QVariant SchemaTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const SchemaColumn *column = SchemaColumns::at(index.column());

    if (role == Qt::TextAlignmentRole)
        return column && column->alignRight ? QVariant(Qt::AlignRight) : QVariant();

    const schema::Field *item = itemFromIndex(index);
    if (!item)
        return {};

    switch (role) {
    case IsAutoPadRole:
        return item->isAutoPad();

    case EditorKindRole:
        return editorKind(index);
    case ChoicesRole:
        return column && column->choices ? QVariant(column->choices(*m_tree, *item)) : QVariant();
    case MinimumRole:
        return 1;
    case MaximumRole:
        return schema::Field::kMaxCount;

    case Qt::DisplayRole:
    case Qt::EditRole:      // o editor padrão da view lê o valor atual pelo EditRole
        return column ? column->read(*m_tree, *item) : QVariant();
    }

    // Os roles de valor (NameRole...) valem em qualquer coluna.
    const SchemaColumn *byRole = SchemaColumns::forRole(role);

    return byRole ? byRole->read(*m_tree, *item) : QVariant();
}

bool SchemaTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    schema::Field *item = itemFromIndex(index);
    if (!item || !m_tree)
        return false;

    const SchemaColumn *column = role == Qt::EditRole ? SchemaColumns::at(index.column())
                                                      : SchemaColumns::forRole(role);

    // O model avisa as views por dataChanged a partir dos sinais do FieldTree.
    return column && column->write && column->write(*m_tree, item, value);
}

Qt::ItemFlags SchemaTreeModel::flags(const QModelIndex &index) const
{
    // O índice inválido é a raiz (invisível): recebe soltura no espaço vazio da view.
    if (!index.isValid())
        return m_tree ? Qt::ItemIsDropEnabled : Qt::NoItemFlags;

    auto *item = itemFromIndex(index);
    if (!item)
        return Qt::NoItemFlags;

    Qt::ItemFlags result = QAbstractItemModel::flags(index);

    if (item->isAutoPad())
        return result;

    // Arrastar e soltar só pelo nome.
    if (index.column() == NameColumn)
        result |= Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;

    // Editável conforme a coluna (o total é calculado; o tipo de um campo com filhos é struct).
    const SchemaColumn *column = SchemaColumns::at(index.column());

    if (!column || column->editor.isEmpty() || !column->write
        || (column->editable && !column->editable(*item)))
        return result;

    return result | Qt::ItemIsEditable;
}

QHash<int, QByteArray> SchemaTreeModel::roleNames() const
{
    static const QHash<int, QByteArray> names{
        { Qt::DisplayRole,  "display" },
        { Qt::EditRole,     "edit" },
        { NameRole,         "name" },
        { TypeIdRole,       "typeId" },
        { CountRole,        "count" },
        { SizeInBytesRole,  "sizeInBytes" },
        { FormatIdRole,     "formatId" },
        { IsAutoPadRole,    "isAutoPad" },
        { EditorKindRole,   "editorKind" },
        { ChoicesRole,      "choices" },
        { MinimumRole,      "minimum" },
        { MaximumRole,      "maximum" }
    };

    return names;
}

schema::Field *SchemaTreeModel::itemFromIndex(const QModelIndex &index) const
{
    if (!m_tree)
        return nullptr;

    if (!index.isValid())
        return m_tree->root();

    return static_cast<schema::Field *>(index.internalPointer());
}

QModelIndex SchemaTreeModel::indexFromItem(const schema::Field *item, int column) const
{
    if (!item || !m_tree)
        return QModelIndex();

    if (item == m_tree->root())
        return QModelIndex();

    return createIndex(item->row(), column, item);
}

void SchemaTreeModel::onFieldChanged(const schema::Field *item)
{
    if (!item)
        return;

    const QModelIndex first = indexFromItem(item, 0);
    const QModelIndex last = indexFromItem(item, ColumnCount - 1);

    if (!first.isValid() || !last.isValid())
        return;

    emit dataChanged(first, last);
}

} // namespace bina::model

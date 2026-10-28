#include "bina/model/ResultTableModel.h"

namespace bina::model {

ResultTableModel::ResultTableModel(QObject *parent)
    : QAbstractTableModel{parent}
{
}

void ResultTableModel::setSource(ResultTreeModel *source)
{
    if (m_source == source)
        return;

    beginResetModel();

    if (m_source)
        disconnect(m_source, nullptr, this, nullptr);

    m_source = source;

    if (m_source) {
        // A fonte recria os nós ao mudar de estrutura; enquanto isso nenhum acesso é permitido.
        connect(m_source, &QAbstractItemModel::modelAboutToBeReset, this, &ResultTableModel::beginResetModel);
        connect(m_source, &QAbstractItemModel::modelReset, this, [this] {
            rebuild();
            endResetModel();
        });

        // Registros novos lidos pela fonte (só há lotes com mais de um registro: então as linhas de
        // topo da fonte são os registros, as mesmas linhas da tabela).
        connect(m_source, &QAbstractItemModel::rowsAboutToBeInserted, this,
                [this](const QModelIndex &parent, int first, int last) {
                    if (!parent.isValid() && !m_source->isSingleRecord())
                        beginInsertRows({}, first, last);
                });
        connect(m_source, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex &parent) {
            if (!parent.isValid() && !m_source->isSingleRecord()) {
                m_rows = m_source->recordCount();
                endInsertRows();
            }
        });

        // Mesma estrutura, novos valores ou formatos. A fonte avisa cada nível da árvore, começando
        // pelo topo; a tabela inteira muda uma vez, no aviso do topo.
        connect(m_source, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft) {
            if (!topLeft.parent().isValid() && m_rows > 0 && !m_columns.isEmpty())
                emit dataChanged(index(0, 0), index(m_rows - 1, int(m_columns.size()) - 1));
        });
    }

    rebuild();
    endResetModel();
}

int ResultTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows;
}

int ResultTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_columns.size());
}

QVariant ResultTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Vertical)
        return section >= 0 && section < m_rows ? tr("registro[%1]").arg(section) : QVariant();

    return section >= 0 && section < m_columns.size() ? QVariant(m_columns.at(section).header) : QVariant();
}

QVariant ResultTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_source)
        return {};

    // Campo ausente (registro truncado sem esse campo) aparece vazio e esmaecido.
    const ResultTreeModel::Node *node = nodeAt(index.row(), index.column());

    switch (role) {
    case Qt::DisplayRole:
        return node ? m_source->formatValue(*node) : QString();
    case Qt::EditRole:
        return node ? m_source->formatValue(*node) : QString();
    case EditorKindRole:
        return node ? QVariant(QString(EditorKinds::kReadOnly)) : QVariant();
    case FormatIdRole:
        return node ? m_source->effectiveFormatId(*node) : QString();
    case TruncatedRole:
        return !node || node->truncated;
    default:
        return {};
    }
}

QHash<int, QByteArray> ResultTableModel::roleNames() const
{
    static const QHash<int, QByteArray> names{
        { Qt::DisplayRole,  "display" },
        { Qt::EditRole,     "edit" },
        { FormatIdRole,     "formatId" },
        { TruncatedRole,    "truncated" },
        { EditorKindRole,   "editorKind" }
    };

    return names;
}

Qt::ItemFlags ResultTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || !nodeAt(index.row(), index.column()))
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

bool ResultTableModel::setData(const QModelIndex &, const QVariant &, int)
{
    return false;
}

bool ResultTableModel::canFetchMore(const QModelIndex &parent) const
{
    return !parent.isValid() && m_source && m_source->canFetchMore({});
}

void ResultTableModel::fetchMore(const QModelIndex &parent)
{
    if (canFetchMore(parent))
        m_source->fetchMore({});
}

QStringList ResultTableModel::compatibleFormats(const QModelIndex &index) const
{
    const ResultTreeModel::Node *node = index.isValid() && m_source ? nodeAt(index.row(), index.column()) : nullptr;

    return node ? m_source->compatibleFormatsFor(*node) : QStringList();
}

bool ResultTableModel::setFormat(const QModelIndex &index, const QString &formatId)
{
    const ResultTreeModel::Node *node = index.isValid() && m_source ? nodeAt(index.row(), index.column()) : nullptr;

    return node && m_source->setNodeFormat(*node, formatId);
}

const ResultTreeModel::Node *ResultTableModel::nodeAt(int row, int column) const
{
    if (!m_source || column < 0 || column >= m_columns.size())
        return nullptr;

    const ResultTreeModel::Node *node = m_source->recordNode(row);

    for (const int step : m_columns.at(column).path) {
        if (!node || step >= node->children.size())
            return nullptr;

        node = &node->children.at(step);
    }

    return node;
}

void ResultTableModel::collectColumns(const ResultTreeModel::Node &parent, const QString &prefix, const QList<int> &path)
{
    for (int i = 0; i < parent.children.size(); ++i) {
        const ResultTreeModel::Node &child = parent.children.at(i);

        const QString name = prefix.isEmpty() ? child.name : prefix + QLatin1Char('.') + child.name;
        const QList<int> childPath = path + QList<int>{i};

        if (child.children.isEmpty() || !m_source->effectiveFormatId(child).isEmpty()) {
            m_columns.append({name, childPath});
            continue;
        }

        // As instâncias de um array de structs já se chamam "nome[i]": o nome do array não se repete.
        const bool arrayOfStructs = child.children.first().instance >= 0;

        collectColumns(child, arrayOfStructs ? prefix : name, childPath);
    }
}

void ResultTableModel::rebuild()
{
    m_columns.clear();
    m_rows = m_source ? m_source->recordCount() : 0;

    if (m_rows > 0)
        collectColumns(*m_source->recordNode(0), {}, {});
}

} // namespace bina::model

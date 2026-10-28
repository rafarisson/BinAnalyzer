#include <algorithm>

#include "bina/model/ResultTreeModel.h"
#include "bina/model/EditorRoles.h"
#include "bina/session/AnalysisSession.h"
#include "bina/document/Document.h"
#include "bina/document/FieldTree.h"

// bina::schema
#include "bina/schema/Field.h"

// bina::format
#include "bina/format/FieldFormat.h"
#include "bina/format/FormatRegistry.h"

namespace bina::model {

ResultTreeModel::ResultTreeModel(QObject *parent)
    : QAbstractItemModel{parent}
{
}

void ResultTreeModel::setSession(session::AnalysisSession *session, Part part)
{
    if (m_session == session && m_part == part)
        return;

    m_part = part;

    beginResetModel();

    if (m_session)
        disconnect(m_session, nullptr, this, nullptr);

    m_session = session;
    m_result = parse::ParseResult();
    m_parents.clear();
    m_overrides.clear();

    if (m_session) {
        connect(m_session, &session::AnalysisSession::invalidated, this, &ResultTreeModel::onInvalidated);
        connect(m_session, &session::AnalysisSession::analyzed, this, &ResultTreeModel::onAnalyzed);
    }

    finishReset();
}

document::Document *ResultTreeModel::document() const
{
    return m_session ? m_session->document() : nullptr;
}

// Os nós antigos deixaram de valer: esquece o resultado e as escolhas de formato.
void ResultTreeModel::onInvalidated()
{
    m_overrides.clear();

    if (m_result.records.empty())
        return;

    beginResetModel();
    m_result = parse::ParseResult();
    m_shownRecords = 0;
    m_parents.clear();
    finishReset();
}

void ResultTreeModel::onAnalyzed()
{
    parse::ParseResult result = m_part == Part::Header ? m_session->takeHeaderResult() : m_session->takeResult();

    // Mesma estrutura: atualiza os nós no lugar para preservar índices e expansão da view.
    if (!m_result.records.empty() && sameShape(m_result, result)) {
        for (size_t i = 0; i < m_result.records.size(); ++i)
            assignInPlace(m_result.records[i], std::move(result.records[i]));

        m_result.generation = result.generation;

        emitAllChanged({});
        return;
    }

    beginResetModel();
    m_result = std::move(result);
    m_shownRecords = m_result.parsedCount();
    m_parents.clear();
    indexRecords(0);
    finishReset();
}

QStringList ResultTreeModel::compatibleFormats(const QModelIndex &index) const
{
    const Node *node = nodeFromIndex(index);

    return node ? compatibleFormatsFor(*node) : QStringList();
}

const ResultTreeModel::Node *ResultTreeModel::recordNode(int index) const
{
    return index >= 0 && index < m_shownRecords ? &m_result.records.at(size_t(index)) : nullptr;
}

QStringList ResultTreeModel::compatibleFormatsFor(const Node &node) const
{
    const document::Document *doc = document();
    if (!doc)
        return {};

    QStringList ids;
    for (const format::Formatter *f : doc->formats()->compatible(node.target))
        ids << f->id();

    return ids;
}

bool ResultTreeModel::setNodeFormat(const Node &node, const QString &formatId)
{
    const document::Document *doc = document();
    if (!doc)
        return false;

    const QString id = formatId.trimmed();

    if (id.isEmpty()) {
        if (m_overrides.remove(node.field->id()) == 0)
            return false;
    } else {
        if (!doc->formats()->isCompatible(id, node.target) || m_overrides.value(node.field->id()) == id)
            return false;

        m_overrides.insert(node.field->id(), id);
    }

    // O formato vale para o campo da estrutura, então todas as instâncias mudam.
    emitAllChanged({});

    return true;
}

void ResultTreeModel::resetFormats()
{
    m_overrides.clear();
    emitAllChanged({});
}

void ResultTreeModel::fetchAll()
{
    // Para também se um lote não acrescentar nada (falha de leitura).
    for (qint64 before = -1; canFetchMore({}) && m_shownRecords != before;) {
        before = m_shownRecords;
        fetchMore({});
    }
}

bool ResultTreeModel::canFetchMore(const QModelIndex &parent) const
{
    return !parent.isValid() && !m_notifying && !isSingleRecord() && m_session
           && m_session->canFetchMore(m_result);
}

// Quem recebe os avisos de reset e de inserção pode pedir fetchMore() ainda dentro deles (as views e
// o QAbstractItemModelTester fazem isso); inserir linhas nesse momento confundiria quem ainda não
// terminou de tratar o aviso anterior. O pedido é recusado e a view pede de novo depois.
void ResultTreeModel::finishReset()
{
    m_notifying = true;
    endResetModel();
    m_notifying = false;
}

void ResultTreeModel::fetchMore(const QModelIndex &parent)
{
    if (!canFetchMore(parent))
        return;

    const qint64 first = m_shownRecords;
    const qint64 count = std::min(session::AnalysisSession::kRecordBatch, m_result.recordCount() - first);

    // Lê antes de anunciar: numa falha o lote pode vir incompleto ou vazio, e a view só deve saber
    // das linhas que existem.
    const qint64 added = m_session->fetchMore(m_result, count);
    if (added <= 0)
        return;

    m_notifying = true;
    beginInsertRows({}, int(first), int(first + added - 1));
    m_shownRecords = first + added;
    indexRecords(first);
    endInsertRows();
    m_notifying = false;
}

const ResultTreeModel::Node *ResultTreeModel::top() const
{
    return isSingleRecord() && !m_result.records.empty() ? &m_result.records.front() : nullptr;
}

int ResultTreeModel::childCount(const Node *parent) const
{
    return parent ? int(parent->children.size()) : int(m_shownRecords);
}

const ResultTreeModel::Node *ResultTreeModel::childAt(const Node *parent, int row) const
{
    if (row < 0 || row >= childCount(parent))
        return nullptr;

    return parent ? &parent->children.at(row) : &m_result.records.at(size_t(row));
}

int ResultTreeModel::rowOf(const Node *node) const
{
    const Node *parent = m_parents.value(node, nullptr);

    return parent ? int(node - parent->children.constData()) : int(node->instance);
}

// Nó que representa um registro inteiro (a estrutura raiz), e não um campo.
bool ResultTreeModel::isRecord(const Node &node) const
{
    // A raiz usada na análise (a estrutura ou a definição do cabeçalho), mesmo que o documento já
    // tenha outra.
    return node.field && node.field == m_result.structure;
}

const ResultTreeModel::Node *ResultTreeModel::nodeFromIndex(const QModelIndex &index) const
{
    return index.isValid() ? static_cast<const Node *>(index.internalPointer()) : nullptr;
}

QModelIndex ResultTreeModel::indexFromNode(const Node *node, int column) const
{
    if (!node || node == top())
        return {};

    return createIndex(rowOf(node), column, node);
}

QModelIndex ResultTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column < 0 || column >= ColumnCount || row < 0 || m_result.records.empty())
        return {};

    const Node *child = childAt(parent.isValid() ? nodeFromIndex(parent) : top(), row);

    return child ? createIndex(row, column, child) : QModelIndex();
}

QModelIndex ResultTreeModel::parent(const QModelIndex &child) const
{
    const Node *node = nodeFromIndex(child);
    if (!node)
        return {};

    const Node *parentNode = m_parents.value(node, nullptr);

    return !parentNode || parentNode == top() ? QModelIndex() : indexFromNode(parentNode, 0);
}

int ResultTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0 || m_result.records.empty())
        return 0;

    return childCount(parent.isValid() ? nodeFromIndex(parent) : top());
}

int ResultTreeModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

bool ResultTreeModel::hasChildren(const QModelIndex &parent) const
{
    return rowCount(parent) > 0;
}

QVariant ResultTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Vertical)
        return section;

    switch (section) {
    case NameColumn:    return tr("Nome");
    case TypeColumn:    return tr("Tipo");
    case OffsetColumn:  return tr("Offset");
    case SizeColumn:    return tr("Bytes");
    case FormatColumn:  return tr("Exibição");
    case ValueColumn:   return tr("Valor");
    }

    return {};
}

QVariant ResultTreeModel::data(const QModelIndex &index, int role) const
{
    const Node *node = nodeFromIndex(index);
    if (!node || !node->field || !document())
        return {};

    const bool editable = index.column() == FormatColumn && !isRecord(*node);
    const bool isCopyable = index.column() == ValueColumn && !isRecord(*node);

    switch (role) {
    case TruncatedRole:
        return node->truncated;
    case FormatIdRole:
        return effectiveFormatId(*node);
    case EditorKindRole:
        if (isCopyable)
            return QString(EditorKinds::kReadOnly);
        return editable ? QVariant(QString(EditorKinds::kCombo)) : QVariant();
    case ChoicesRole:
        return editable ? QVariant(makeChoices(compatibleFormatsFor(*node), true)) : QVariant();
    case Qt::TextAlignmentRole:
        return index.column() == OffsetColumn || index.column() == SizeColumn
                   ? QVariant(Qt::AlignRight | Qt::AlignVCenter) : QVariant();
    case Qt::EditRole:
        if (isCopyable)
            return formatValue(*node);
        return index.column() == FormatColumn ? QVariant(effectiveFormatId(*node)) : QVariant();
    case Qt::DisplayRole:
        break;
    default:
        return {};
    }

    switch (index.column()) {
    case NameColumn:
        return isRecord(*node) ? tr("registro[%1]").arg(node->instance) : node->name;
    case TypeColumn:
        if (isRecord(*node))
            return tr("registro");

        return node->instance < 0 && node->field->count() > 1
                   ? QStringLiteral("%1[%2]").arg(node->field->typeId()).arg(node->field->count())
                   : node->field->typeId();
    case OffsetColumn:
        return QStringLiteral("0x%1").arg(node->offset, 4, 16, QLatin1Char('0')).toUpper().replace(1, 1, QLatin1Char('x'));
    case SizeColumn:
        return node->size;
    case FormatColumn:
        return effectiveFormatId(*node);
    case ValueColumn:
        return formatValue(*node);
    }

    return {};
}

bool ResultTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    const Node *node = nodeFromIndex(index);
    if (!node || role != Qt::EditRole || index.column() != FormatColumn)
        return false;

    return setNodeFormat(*node, value.toString());
}

Qt::ItemFlags ResultTreeModel::flags(const QModelIndex &index) const
{
    if (!nodeFromIndex(index))
        return Qt::NoItemFlags;

    Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    // O valor formatado "abre" num editor para ser copiado; setData() recusa a gravação.
    if ((index.column() == FormatColumn || index.column() == ValueColumn) && !isRecord(*nodeFromIndex(index)))
        result |= Qt::ItemIsEditable;

    return result;
}

QHash<int, QByteArray> ResultTreeModel::roleNames() const
{
    static const QHash<int, QByteArray> names{
        { Qt::DisplayRole,  "display" },
        { Qt::EditRole,     "edit" },
        { TruncatedRole,    "truncated" },
        { FormatIdRole,     "formatId" },
        { EditorKindRole,   "editorKind" },
        { ChoicesRole,      "choices" }
    };

    return names;
}

// Com a estrutura alterada depois da análise, o formato do campo pode não servir mais ao valor
// que foi lido: nesse caso o valor continua sendo exibido no formato padrão do tipo original.
QString ResultTreeModel::effectiveFormatId(const Node &node) const
{
    const document::Document *doc = document();
    const format::FormatRegistry *formats = doc->formats();

    QString id;
    if (const auto it = m_overrides.constFind(node.field->id()); it != m_overrides.constEnd())
        id = it.value();
    else
        id = format::effectiveFormatId(*node.field, *doc->types(), *formats);

    return id.isEmpty() || formats->isCompatible(id, node.target) ? id : formats->defaultId(node.target);
}

QString ResultTreeModel::formatValue(const Node &node) const
{
    if (!node.value.isValid())
        return node.truncated ? QStringLiteral("—") : QString();

    const QString id = effectiveFormatId(node);
    if (id.isEmpty())
        return {};

    return document()->formats()->format(id, node.target, node.value);
}

void ResultTreeModel::indexParents(const Node &parent)
{
    for (const Node &child : parent.children) {
        m_parents.insert(&child, &parent);
        indexParents(child);
    }
}

void ResultTreeModel::indexRecords(qint64 first)
{
    for (qint64 i = first; i < m_result.parsedCount(); ++i)
        indexParents(m_result.records.at(size_t(i)));
}

void ResultTreeModel::emitAllChanged(const QModelIndex &parent)
{
    const int rows = rowCount(parent);
    if (rows == 0)
        return;

    emit dataChanged(index(0, 0, parent), index(rows - 1, ColumnCount - 1, parent));

    for (int row = 0; row < rows; ++row)
        emitAllChanged(index(row, 0, parent));
}

bool ResultTreeModel::sameShape(const parse::ParseResult &a, const parse::ParseResult &b)
{
    if (a.structure != b.structure || a.records.size() != b.records.size()
        || a.recordSize != b.recordSize || a.dataSize != b.dataSize || a.dataOffset != b.dataOffset)
        return false;

    for (size_t i = 0; i < a.records.size(); ++i)
        if (!sameShape(a.records[i], b.records[i]))
            return false;

    return true;
}

bool ResultTreeModel::sameShape(const Node &a, const Node &b)
{
    if (a.field != b.field || a.name != b.name || a.children.size() != b.children.size())
        return false;

    for (qsizetype i = 0; i < a.children.size(); ++i)
        if (!sameShape(a.children.at(i), b.children.at(i)))
            return false;

    return true;
}

void ResultTreeModel::assignInPlace(Node &dst, Node &&src)
{
    dst.instance = src.instance;
    dst.offset = src.offset;
    dst.size = src.size;
    dst.truncated = src.truncated;
    dst.value = std::move(src.value);
    dst.target = src.target;

    for (qsizetype i = 0; i < dst.children.size(); ++i)
        assignInPlace(dst.children.data()[i], std::move(src.children.data()[i]));
}

} // namespace bina::model

#include "bina/app/MessageLog.h"

namespace bina::app {

MessageLog::MessageLog(QObject *parent)
    : QAbstractListModel{parent}
{
}

void MessageLog::add(const QString &text, bool isError)
{
    if (m_entries.size() >= kMaxMessages) {
        beginRemoveRows({}, 0, 0);
        m_entries.removeFirst();
        endRemoveRows();
    }

    const int row = int(m_entries.size());

    beginInsertRows({}, row, row);
    m_entries.append({text, QDateTime::currentDateTime(), isError});
    endInsertRows();

    emit countChanged();
    emit messageAdded(text, isError);
}

void MessageLog::clear()
{
    if (m_entries.isEmpty())
        return;

    beginResetModel();
    m_entries.clear();
    endResetModel();

    emit countChanged();
}

QString MessageLog::text(int row) const
{
    return row >= 0 && row < m_entries.size() ? m_entries.at(row).text : QString();
}

QStringList MessageLog::lines() const
{
    QStringList out;
    for (const Entry &entry : m_entries)
        out << lineOf(entry);
    return out;
}

int MessageLog::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant MessageLog::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};

    const Entry &entry = m_entries.at(index.row());

    switch (role) {
    case Qt::DisplayRole:   return lineOf(entry);
    case TextRole:          return entry.text;
    case TimeRole:          return entry.time;
    case IsErrorRole:       return entry.isError;
    default:                return {};
    }
}

QHash<int, QByteArray> MessageLog::roleNames() const
{
    static const QHash<int, QByteArray> names{
        { Qt::DisplayRole,  "display" },
        { TextRole,         "text" },
        { TimeRole,         "time" },
        { IsErrorRole,      "isError" }
    };

    return names;
}

QString MessageLog::lineOf(const Entry &entry)
{
    return QStringLiteral("[%1] %2").arg(entry.time.toString(QStringLiteral("HH:mm:ss")), entry.text);
}

} // namespace bina::app

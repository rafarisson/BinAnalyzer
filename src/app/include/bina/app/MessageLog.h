#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

namespace bina::app {

// Histórico das mensagens para o usuário (avisos do carregamento, recusas de edição, erros), da mais
// antiga à mais nova, como um model de lista para o QML. Guarda as últimas kMaxMessages.
class MessageLog : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Criado pelo AppController")

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:

    static constexpr int kMaxMessages = 200;

    enum Role {
        TextRole = Qt::UserRole + 1,
        TimeRole,
        IsErrorRole
    };

    explicit MessageLog(QObject *parent = nullptr);

    void add(const QString &text, bool isError = false);
    Q_INVOKABLE void clear();

    int count() const { return int(m_entries.size()); }

    // O texto da mensagem 'row' (sem a hora); vazio fora do intervalo.
    QString text(int row) const;

    // As mensagens como exibidas: "[hh:mm:ss] texto".
    QStringList lines() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:

    void countChanged();

    // Uma mensagem nova (para um aviso rápido na janela).
    void messageAdded(const QString &text, bool isError);

private:

    struct Entry {
        QString text;
        QDateTime time;
        bool isError{false};
    };

    static QString lineOf(const Entry &entry);

    QList<Entry> m_entries;
};

} // namespace bina::app

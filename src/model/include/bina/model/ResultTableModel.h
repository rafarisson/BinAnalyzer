#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QPointer>

#include "bina/model/EditorRoles.h"
#include "bina/model/ResultTreeModel.h"

namespace bina::model {

// Os mesmos dados de ResultTreeModel em forma de tabela: uma linha por registro e uma coluna por
// campo, com o conteúdo já formatado. Os grupos (structs sem formato) são expandidos em colunas
// "grupo.campo"; structs com formato (ex.: um vec3 com padrão printf) ocupam uma única coluna.
// As colunas seguem a estrutura do primeiro registro e são recriadas quando ela muda.
class ResultTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    enum Role {
        FormatIdRole = Qt::UserRole + 1,
        TruncatedRole
    };
    Q_ENUM(Role)

    explicit ResultTableModel(QObject *parent = nullptr);

    // A fonte precisa sobreviver ao model; trocar o formato aqui altera a fonte (e a árvore também).
    void setSource(ResultTreeModel *source);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // As células abrem num editor só para copiar o valor; setData() recusa a gravação.
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // Os registros são lidos em lotes pela fonte; a tabela pede os seguintes do mesmo jeito.
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    // Formatos que podem ser aplicados à célula; id vazio em setFormat() volta ao formato padrão.
    Q_INVOKABLE QStringList compatibleFormats(const QModelIndex &index) const;
    Q_INVOKABLE bool setFormat(const QModelIndex &index, const QString &formatId);

private:

    struct Column {
        QString header;
        QList<int> path;    // posição do campo dentro do registro, de pai para filho
    };

    const ResultTreeModel::Node *nodeAt(int row, int column) const;
    void collectColumns(const ResultTreeModel::Node &parent, const QString &prefix, const QList<int> &path);
    void rebuild();

    QPointer<ResultTreeModel> m_source;
    QList<Column> m_columns;
    int m_rows{0};
};

} // namespace bina::model

#pragma once

#include <QAbstractItemModel>
#include <QVariant>

#include "bina/model/PendingMoveGuard.h"

namespace bina::schema { class Field; }
namespace bina::document { class FieldTree; }

namespace bina::model {

// A árvore de campos da estrutura (document::FieldTree) como model de itens: uma linha por campo,
// com a raiz como único item de topo.
//
// Edição, menu e arrastar e soltar são resolvidos aqui, pelo protocolo padrão do Qt (flags,
// setData, insertRows/removeRows/moveRows, mimeData/canDropMimeData/dropMimeData) e por uma API
// em índices para quem acompanha o ponteiro (actions/trigger, canDropAt/dropAt). A view só
// precisa dos gestos e do desenho.
//
// A implementação está dividida por assunto:
//   SchemaTreeModel.cpp         estrutura, dados, flags e roles (regras por coluna: SchemaColumns)
//   SchemaTreeModelEditing.cpp  ações de menu, insertRows/removeRows/moveRows e soltura por índices
//   SchemaTreeModelDragDrop.cpp protocolo de mime para arrastar e soltar (DropTargetResolver,
//                               PendingMoveGuard)
class SchemaTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    enum Role {
        NameRole = Qt::UserRole + 1,
        TypeIdRole,
        CountRole,
        SizeInBytesRole,
        FormatIdRole,
        IsAutoPadRole
        // EditorKindRole, ChoicesRole, MinimumRole e MaximumRole: ver EditorRoles.h
    };

    enum Column {
        NameColumn = 0,
        TypeIdColumn,
        CountColumn,
        SizeInBytesColumn,
        FormatIdColumn,

        ColumnCount
    };
    Q_ENUM(Column)

    // Onde soltar em relação ao alvo. Os valores coincidem com document::FieldTree::DropZone.
    enum DropZone {
        Before,
        Onto,
        After
    };
    Q_ENUM(DropZone)

    explicit SchemaTreeModel(QObject *parent = nullptr);
    explicit SchemaTreeModel(document::FieldTree *model, QObject *parent = nullptr);

    void setModel(document::FieldTree *model);
    const document::FieldTree *model() const { return m_tree; }

    const schema::Field *root() const;

    // Menu de contexto genérico: lista de {id, text, enabled} para o campo e execução por id.
    // As regras (o que está habilitado, auto_pad sem ações) ficam aqui, não na view.
    Q_INVOKABLE QVariantList actions(const QModelIndex &index) const;
    Q_INVOKABLE bool trigger(const QModelIndex &index, const QString &actionId);

    // Arrastar e soltar por índices, com as mesmas regras de canDropMimeData/dropMimeData, para
    // um item que acompanha o ponteiro (QML). Não usa QMimeData e não depende de removeRows().
    Q_INVOKABLE bool canDropAt(const QModelIndex &source, const QModelIndex &target, DropZone zone) const;
    Q_INVOKABLE bool dropAt(const QModelIndex &source, const QModelIndex &target, DropZone zone);

    // QAbstractItemModel: posição e tamanhos

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    // QAbstractItemModel: leitura e escrita de valores

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // QAbstractItemModel: manipulação de item

    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;

    // QAbstractItemModel: flags

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;

    // QAbstractItemModel: drag/drop

    QStringList mimeTypes() const override;
    bool canDropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    // QAbstractItemModel: roles

    QHash<int, QByteArray> roleNames() const override;

signals:

    // O campo deve ficar visível (a view pode expandir os pais, selecioná-lo ou abrir a edição):
    // emitido para o campo criado por uma ação e para o campo movido por uma soltura.
    void revealRequested(const QModelIndex &index);

    // Uma edição foi recusada; 'message' explica o motivo.
    void errorOccurred(const QString &message);

private:

    schema::Field *sourceFromMime(const QMimeData *data) const;

    QString editorKind(const QModelIndex &index) const;

    schema::Field *itemFromIndex(const QModelIndex &index) const;
    QModelIndex indexFromItem(const schema::Field *item, int column = 0) const;
    void onFieldChanged(const schema::Field *item);

    document::FieldTree *m_tree{nullptr};
    quint64 m_token{0};

    // Ignora o removeRows() que a view manda depois de um drop com MoveAction.
    PendingMoveGuard m_pendingMove;
};

} // namespace bina::model

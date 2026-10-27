#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QPointer>

#include "bina/session/AnalysisSession.h"
#include "bina/parse/ParsedNode.h"
#include "bina/schema/Field.h"

namespace bina::document { class Document; }
namespace bina::schema { class Field; }

namespace bina::model {

// Árvore dos dados analisados: uma linha por campo do binário, com os valores formatados sob
// demanda. Exibe o resultado de uma AnalysisSession: os nós entregues pela sessão pertencem a
// este model, que os atualiza no lugar quando a estrutura não mudou (preservando índices e a
// expansão da view) e os recria quando mudou.
//
// Trocar o formato de um campo não reanalisa o arquivo; a escolha vale para o campo da estrutura
// (todas as instâncias) e é esquecida quando o resultado é descartado.
//
// Com mais de um registro, as linhas de topo são os registros, lidos em lotes conforme a view pede
// (canFetchMore()/fetchMore()); fetchAll() lê os que faltam, por exemplo para exportar.
class ResultTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    enum Role {
        TruncatedRole = Qt::UserRole + 1,
        FormatIdRole
    };

    enum Column {
        NameColumn = 0,
        TypeColumn,
        OffsetColumn,
        SizeColumn,
        FormatColumn,
        ValueColumn,

        ColumnCount
    };
    Q_ENUM(Column)

    using Node = parse::ParsedNode;

    explicit ResultTreeModel(QObject *parent = nullptr);

    // A sessão (e o documento dela) precisam sobreviver ao model.
    void setSession(session::AnalysisSession *session);

    // Acesso por nó, para outras visões dos mesmos dados (ver ResultTableModel). Os ponteiros valem
    // até o próximo modelReset(); ler mais registros não os invalida.
    // recordCount() conta os registros já lidos; totalRecords(), os do arquivo.
    int recordCount() const { return int(m_shownRecords); }
    qint64 totalRecords() const { return m_result.recordCount(); }
    bool isSingleRecord() const { return m_result.recordCount() == 1; }
    const Node *recordNode(int index) const;
    QString effectiveFormatId(const Node &node) const;
    QString formatValue(const Node &node) const;
    QStringList compatibleFormatsFor(const Node &node) const;

    // id vazio remove a escolha e volta ao formato padrão.
    bool setNodeFormat(const Node &node, const QString &formatId);

    Q_INVOKABLE QStringList compatibleFormats(const QModelIndex &index) const;
    Q_INVOKABLE void resetFormats();

    // Lê o próximo lote de registros, ou todos os que faltam (se a sessão ainda permitir).
    Q_INVOKABLE void fetchMoreRecords() { fetchMore({}); }
    void fetchAll();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

private:

    // Com um único registro, as linhas de topo são os campos dele (top() é esse registro); senão são
    // os registros (top() é nullptr). Os registros não têm pai em m_parents.
    const Node *top() const;
    int childCount(const Node *parent) const;
    const Node *childAt(const Node *parent, int row) const;
    int rowOf(const Node *node) const;
    bool isRecord(const Node &node) const;

    const Node *nodeFromIndex(const QModelIndex &index) const;
    QModelIndex indexFromNode(const Node *node, int column = 0) const;
    document::Document *document() const;

    void finishReset();
    void onInvalidated();
    void onAnalyzed();
    void indexParents(const Node &parent);
    void indexRecords(qint64 first);
    void emitAllChanged(const QModelIndex &parent);

    static bool sameShape(const Node &a, const Node &b);
    static bool sameShape(const parse::ParseResult &a, const parse::ParseResult &b);
    static void assignInPlace(Node &dst, Node &&src);

    QPointer<session::AnalysisSession> m_session;
    bool m_notifying{false};       // emitindo modelReset()/rowsInserted() (ver finishReset())

    parse::ParseResult m_result;

    // Registros já anunciados às views. Fica atrás de m_result.parsedCount() só durante fetchMore(),
    // entre a leitura e o rowsInserted, para rowCount() não mudar antes de beginInsertRows().
    qint64 m_shownRecords{0};
    QHash<const Node *, const Node *> m_parents;

    // Formato escolhido pelo usuário por campo da estrutura; vale para todas as instâncias do campo.
    // A chave é o id do campo, que não é reaproveitado: uma escolha nunca passa a outro campo.
    QHash<schema::Field::Id, QString> m_overrides;
};

} // namespace bina::model

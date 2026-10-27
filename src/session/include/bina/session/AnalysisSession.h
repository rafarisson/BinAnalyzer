#pragma once

#include <memory>

#include <QByteArray>
#include <QByteArrayView>
#include <QFile>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>

#include "bina/document/Document.h"
#include "bina/parse/ParsedNode.h"

namespace bina::schema { class Field; }

namespace bina::session {

// Uma análise em andamento: os bytes de um arquivo lidos segundo a estrutura de um Document.
//
// A sessão guarda os bytes, executa o BinaryParser e acompanha as alterações da estrutura. Ela não
// guarda os nós do resultado: entrega-os uma vez (takeResult()) a quem vai exibi-los, que passa a ser
// dono deles. Isso mantém a análise independente de qualquer view.
//
// Os registros são lidos sob demanda: takeResult() traz o primeiro lote (kRecordBatch registros) e
// quem exibe pede os seguintes com fetchMore(), conforme a view precisa. Assim um arquivo grande não
// trava a interface nem ocupa memória com nós que ninguém viu. O arquivo é mapeado em memória
// quando possível, em vez de lido inteiro.
//
// Alterar a estrutura não reanalisa o arquivo sozinho: a sessão passa a ser "desatualizada"
// (isStale()) e a reanálise é pedida com reanalyze(). Um documento novo é analisado de imediato.
// Enquanto desatualizada, a sessão não lê mais registros: eles teriam outra estrutura.
class AnalysisSession : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool hasData READ hasData NOTIFY statusChanged)
    Q_PROPERTY(bool stale READ isStale NOTIFY statusChanged)
    Q_PROPERTY(qint64 recordCount READ recordCount NOTIFY statusChanged)
    Q_PROPERTY(qint64 completeRecords READ completeRecords NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString error READ error NOTIFY statusChanged)

public:

    // Registros lidos de uma vez: o primeiro lote e cada fetchMore() de quem exibe.
    static constexpr qint64 kRecordBatch = 1000;

    explicit AnalysisSession(QObject *parent = nullptr);
    ~AnalysisSession() override;

    // O documento fornece estrutura, tipos, formatos e ordem de bytes, e precisa sobreviver à sessão.
    void setDocument(document::Document *document);
    document::Document *document() const { return m_document; }

    bool hasData() const { return m_hasData; }
    bool isStale() const { return m_stale; }
    QString error() const { return m_error; }
    QString statusText() const;

    // Registros do último resultado (inclui o último, se incompleto) e os que estão inteiros.
    // Contam o arquivo todo, não só os registros já lidos.
    qint64 recordCount() const { return m_hasResult ? m_summary.recordCount : 0; }
    qint64 completeRecords() const;

    // Caminho local ou URL "file:". Em caso de falha devolve false e error() diz o motivo.
    bool loadFile(const QString &path);
    void setBytes(const QByteArray &bytes);
    void clear();

    // Analisa de novo os mesmos bytes com a estrutura atual.
    void reanalyze();

    // O resultado entregue por analyzed(), com o primeiro lote de registros. Vale uma vez por sinal.
    parse::ParseResult takeResult();

    // 'result' (entregue por esta sessão) ainda pode receber registros: é da análise atual, a
    // estrutura não mudou desde então, faltam registros e nenhuma leitura falhou.
    bool canFetchMore(const parse::ParseResult &result) const;

    // Acrescenta a 'result' até 'count' registros. Devolve quantos foram acrescentados, que podem
    // ser menos que 'count' (ou nenhum) se a leitura falhar; error() diz o motivo.
    qint64 fetchMore(parse::ParseResult &result, qint64 count = kRecordBatch);

signals:

    // Os nós entregues antes deixaram de valer (campos removidos, documento novo ou falha).
    void invalidated();

    // Há um resultado novo para takeResult().
    void analyzed();

    void statusChanged();

private:

    struct Summary {
        qint64 recordSize{0};
        qint64 dataSize{0};
        qint64 recordCount{0};
    };

    QByteArrayView data() const;
    void releaseFile();

    void invalidate();
    void markStale();
    void onAboutToRemove(const schema::Field *parent, int first, int last);
    void scheduleReparse();
    void reparse();

    QPointer<document::Document> m_document;

    // Os bytes analisados: o arquivo mapeado (m_file + m_mapped) ou uma cópia em m_bytes.
    std::unique_ptr<QFile> m_file;
    const uchar *m_mapped{nullptr};
    qint64 m_mappedSize{0};
    QByteArray m_bytes;

    bool m_hasData{false};
    bool m_hasResult{false};
    bool m_stale{false};
    bool m_fetchFailed{false};     // um fetchMore() falhou nesta análise
    QString m_error;
    QTimer m_reparseTimer;      // agrupa alterações seguidas em uma análise; reparse() o cancela

    quint64 m_generation{0};    // muda a cada análise; identifica os resultados entregues
    Summary m_summary;
    parse::ParseResult m_pending;
};

} // namespace bina::session

#include <QTimer>
#include <QUrl>

#include "bina/session/AnalysisSession.h"
#include "bina/parse/BinaryParser.h"
#include "bina/document/Document.h"
#include "bina/document/FieldTree.h"
#include "bina/schema/LayoutCalculator.h"

namespace bina::session {

AnalysisSession::AnalysisSession(QObject *parent)
    : QObject{parent}
{
    m_reparseTimer.setSingleShot(true);
    m_reparseTimer.setInterval(0);
    connect(&m_reparseTimer, &QTimer::timeout, this, &AnalysisSession::reparse);
}

AnalysisSession::~AnalysisSession()
{
    releaseFile();
}

void AnalysisSession::setDocument(document::Document *document)
{
    if (m_document == document)
        return;

    if (m_document) {
        disconnect(m_document->fields(), nullptr, this, nullptr);
        disconnect(m_document, nullptr, this, nullptr);
    }

    m_document = document;
    invalidate();

    if (!m_document)
        return;

    const document::FieldTree *tree = m_document->fields();

    // Um documento novo é analisado de imediato. Alterações na estrutura só marcam os dados como
    // desatualizados. Os nós guardam ponteiros para os campos, então só a remoção de campos (ou um
    // documento novo) descarta o resultado já entregue.
    connect(tree, &document::FieldTree::aboutToReset, this, &AnalysisSession::invalidate);
    connect(tree, &document::FieldTree::reset, this, &AnalysisSession::scheduleReparse);
    connect(tree, &document::FieldTree::aboutToRemove, this, &AnalysisSession::onAboutToRemove);

    connect(tree, &document::FieldTree::inserted, this, &AnalysisSession::markStale);
    connect(tree, &document::FieldTree::removed, this, &AnalysisSession::markStale);
    connect(tree, &document::FieldTree::moved, this, &AnalysisSession::markStale);
    connect(tree, &document::FieldTree::fieldChanged, this, &AnalysisSession::markStale);

    connect(m_document, &document::Document::byteOrderChanged, this, &AnalysisSession::markStale);
    connect(m_document, &document::Document::maxAlignChanged, this, &AnalysisSession::markStale);
    connect(m_document, &document::Document::formatsChanged, this, &AnalysisSession::markStale);

    // Os nós apontam para os campos das definições dos tipos: ao trocá-las o resultado é descartado.
    connect(m_document, &document::Document::typesAboutToChange, this, &AnalysisSession::invalidate);
    connect(m_document, &document::Document::typeDefinitionChanged, this, &AnalysisSession::markStale);

    scheduleReparse();
}

QString AnalysisSession::statusText() const
{
    if (!m_error.isEmpty())
        return m_error;

    if (!m_hasData)
        return tr("Nenhum arquivo carregado");

    // Campos removidos descartam o resultado até a próxima análise.
    if (!m_hasResult)
        return tr("Resultado descartado: a estrutura mudou. Analise o arquivo novamente.");

    const qint64 complete = m_summary.recordSize > 0 ? m_summary.dataSize / m_summary.recordSize : 0;
    const qint64 partial = m_summary.recordSize > 0 ? m_summary.dataSize % m_summary.recordSize : 0;

    if (m_summary.dataSize == 0)
        return tr("Arquivo vazio (registro de %1 bytes)").arg(m_summary.recordSize);

    if (partial == 0 && complete == 1)
        return tr("1 registro (%1 bytes)").arg(m_summary.recordSize);

    if (partial == 0)
        return tr("%1 registros de %2 bytes (%3 bytes)")
                   .arg(complete).arg(m_summary.recordSize).arg(m_summary.dataSize);

    if (complete == 0)
        return tr("Registro incompleto: %1 de %2 bytes").arg(partial).arg(m_summary.recordSize);

    return tr("%1 registros completos + 1 incompleto (%2 de %3 bytes)")
               .arg(complete).arg(partial).arg(m_summary.recordSize);
}

qint64 AnalysisSession::completeRecords() const
{
    return m_hasResult && m_summary.recordSize > 0 ? m_summary.dataSize / m_summary.recordSize : 0;
}

bool AnalysisSession::loadFile(const QString &path)
{
    const QString local = path.startsWith(QLatin1String("file:")) ? QUrl(path).toLocalFile() : path;

    auto file = std::make_unique<QFile>(local);
    if (!file->open(QIODevice::ReadOnly)) {
        m_error = file->errorString();
        emit statusChanged();
        return false;
    }

    releaseFile();

    // Mapear evita copiar o arquivo inteiro para a memória; arquivos vazios (ou recursos) não
    // podem ser mapeados e são lidos.
    const qint64 size = file->size();
    if (const uchar *mapped = size > 0 ? file->map(0, size) : nullptr) {
        m_file = std::move(file);
        m_mapped = mapped;
        m_mappedSize = size;
    } else {
        m_bytes = file->readAll();
    }

    m_hasData = true;
    reparse();

    return m_error.isEmpty();
}

void AnalysisSession::setBytes(const QByteArray &bytes)
{
    releaseFile();
    m_bytes = bytes;
    m_hasData = true;
    reparse();
}

void AnalysisSession::clear()
{
    releaseFile();
    m_hasData = false;
    reparse();
}

void AnalysisSession::reanalyze()
{
    reparse();
}

parse::ParseResult AnalysisSession::takeResult()
{
    return std::exchange(m_pending, parse::ParseResult());
}

bool AnalysisSession::canFetchMore(const parse::ParseResult &result) const
{
    return m_hasResult && !m_stale && m_document && result.generation == m_generation
           && result.structure == m_document->fields()->root() && !result.isComplete() && !m_fetchFailed;
}

qint64 AnalysisSession::fetchMore(parse::ParseResult &result, qint64 count)
{
    if (!canFetchMore(result) || count <= 0)
        return 0;

    const qint64 before = result.parsedCount();

    parse::BinaryParser parser(m_document->types(), m_document->byteOrder(), m_document->maxAlign());
    if (const schema::OperationResult more = parser.parseMore(result, data(), count); !more) {
        m_error = more.error();
        m_fetchFailed = true;
        emit statusChanged();
    }

    return result.parsedCount() - before;
}

QByteArrayView AnalysisSession::data() const
{
    return m_mapped ? QByteArrayView(m_mapped, m_mappedSize) : QByteArrayView(m_bytes);
}

void AnalysisSession::releaseFile()
{
    if (m_file && m_mapped)
        m_file->unmap(const_cast<uchar *>(m_mapped));

    m_file.reset();
    m_mapped = nullptr;
    m_mappedSize = 0;
    m_bytes.clear();
}

// Descarta o resultado já entregue: quem o exibe precisa esquecer os nós antigos.
void AnalysisSession::invalidate()
{
    if (!m_hasResult)
        return;

    m_hasResult = false;
    m_summary = Summary();
    m_stale = m_hasData;

    emit invalidated();
    emit statusChanged();
}

void AnalysisSession::markStale()
{
    if (!m_hasData || m_stale)
        return;

    m_stale = true;
    emit statusChanged();
}

// Campos removidos deixariam ponteiros inválidos nos nós: o resultado é descartado e só
// reaparece com reanalyze().
void AnalysisSession::onAboutToRemove(const schema::Field *parent, int first, int last)
{
    for (int i = first; i <= last; ++i) {
        if (!parent->child(i)->isAutoPad()) {
            invalidate();
            return;
        }
    }
}

// Várias alterações seguidas resultam em uma única reanálise.
void AnalysisSession::scheduleReparse()
{
    if (!m_reparseTimer.isActive())
        m_reparseTimer.start();
}

void AnalysisSession::reparse()
{
    m_reparseTimer.stop();
    m_stale = false;
    m_pending = parse::ParseResult();
    m_fetchFailed = false;
    ++m_generation;

    bool ok = false;

    if (m_document && m_hasData) {
        parse::BinaryParser parser(m_document->types(), m_document->byteOrder(), m_document->maxAlign());
        const schema::OperationResult parsed = parser.parse(m_document->fields()->root(), data(), m_pending, kRecordBatch);
        ok = parsed.ok();
        m_error = parsed.error();
        m_pending.generation = m_generation;
    } else {
        m_error.clear();
    }

    if (ok) {
        m_summary = {m_pending.recordSize, m_pending.dataSize, m_pending.recordCount()};
        m_hasResult = true;

        emit analyzed();
    } else if (m_hasResult) {
        m_hasResult = false;
        m_summary = Summary();

        emit invalidated();
    }

    emit statusChanged();
}

} // namespace bina::session

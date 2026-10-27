#include <QFileInfo>

#include "bina/app/AppController.h"
#include "bina/app/Paths.h"
#include "bina/document/FieldTree.h"

namespace bina::app {

AppController::AppController(QObject *parent)
    : QObject{parent}
    , m_documents{m_document, m_log}
    , m_exports{m_resultModel, m_tableModel, m_log}
{
    m_schemaModel.setModel(m_document.fields());
    m_session.setDocument(&m_document);
    m_resultModel.setSession(&m_session);
    m_tableModel.setSource(&m_resultModel);
    m_typeController.setDocument(&m_document);
    m_formatController.setDocument(&m_document);

    connect(&m_documents, &DocumentService::pathChanged, this, [this] {
        emit schemaPathChanged();
        emit titleChanged();
    });
    connect(&m_documents, &DocumentService::dirtyChanged, this, [this] {
        emit dirtyChanged();
        emit titleChanged();
    });

    connect(&m_document, &document::Document::byteOrderChanged, this, &AppController::optionsChanged);
    connect(&m_document, &document::Document::maxAlignChanged, this, &AppController::optionsChanged);

    connect(&m_log, &MessageLog::messageAdded, this, &AppController::messageAdded);

    connectMessages();
}

// Avisos para o usuário sobre o que acontece no documento, nos editores e na análise.
void AppController::connectMessages()
{
    const auto error = [this](const QString &message) { m_log.add(message, true); };

    connect(&m_document, &document::Document::formatsChanged, this, [this] {
        m_log.add(m_dataPath.isEmpty()
                      ? tr("Formato alterado.")
                      : tr("Formato alterado: analise o arquivo novamente para atualizar os dados."));
    });
    connect(&m_document, &document::Document::formatAdded, this, [this](const QString &id) {
        m_log.add(tr("Formato '%1' criado.").arg(id));
    });
    connect(&m_document, &document::Document::typeAdded, this, [this](const QString &id) {
        m_log.add(tr("Tipo '%1' criado.").arg(id));
    });
    connect(&m_document, &document::Document::typeDefinitionChanged, this, [this](const QString &id) {
        m_log.add(m_dataPath.isEmpty()
                      ? tr("Tipo '%1' alterado.").arg(id)
                      : tr("Tipo '%1' alterado: analise o arquivo novamente para atualizar os dados.").arg(id));
    });

    connect(&m_formatController, &presentation::FormatPatternController::errorOccurred, this, error);
    connect(&m_typeController, &presentation::TypeDefinitionController::errorOccurred, this, error);
    connect(m_typeController.draftModel(), &model::SchemaTreeModel::errorOccurred, this, error);
    connect(&m_schemaModel, &model::SchemaTreeModel::errorOccurred, this, error);

    // Falhas da análise (por exemplo, estrutura vazia) viram mensagem uma única vez. As falhas ao
    // abrir o arquivo são reportadas por openData(), com o caminho.
    connect(&m_session, &session::AnalysisSession::statusChanged, this, [this] {
        const QString error = m_session.error();

        if (m_loadingData) {
            m_lastSessionError = error;
            return;
        }

        if (error != m_lastSessionError) {
            m_lastSessionError = error;

            if (!error.isEmpty())
                m_log.add(error, true);
        }
    });
}

QString AppController::windowTitle() const
{
    const QString name = schemaPath().isEmpty() ? tr("Sem título") : QFileInfo(schemaPath()).fileName();

    return tr("BinAnalyzer — %1%2").arg(name, isDirty() ? QStringLiteral("*") : QString());
}

void AppController::setBigEndian(bool big)
{
    m_document.setByteOrder(big ? QSysInfo::BigEndian : QSysInfo::LittleEndian);
}

void AppController::setMaxAlign(int align)
{
    if (!m_document.setMaxAlign(align))
        m_log.add(tr("Não foi possível usar o alinhamento %1 com os tipos atuais.").arg(align), true);
}

bool AppController::newSchema()
{
    return m_documents.newDocument();
}

bool AppController::openSchema(const QString &pathOrUrl)
{
    return m_documents.open(pathOrUrl);
}

bool AppController::saveSchema()
{
    return m_documents.save();
}

bool AppController::saveSchemaAs(const QString &pathOrUrl)
{
    return m_documents.saveAs(pathOrUrl);
}

bool AppController::exportResult(const QString &pathOrUrl)
{
    return m_exports.exportResult(pathOrUrl);
}

bool AppController::openData(const QString &pathOrUrl)
{
    const QString path = localPath(pathOrUrl);

    m_loadingData = true;
    const bool loaded = m_session.loadFile(path);
    m_loadingData = false;

    if (!loaded) {
        m_log.add(tr("Não foi possível abrir '%1': %2").arg(path, m_session.error()), true);
        return false;
    }

    m_dataPath = path;
    emit dataPathChanged();

    return true;
}

void AppController::reanalyze()
{
    m_session.reanalyze();
}

void AppController::clearMessages()
{
    m_log.clear();
}

} // namespace bina::app

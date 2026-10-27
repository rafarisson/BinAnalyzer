#include <QFileInfo>

#include "bina/app/DocumentService.h"
#include "bina/app/MessageLog.h"
#include "bina/app/Paths.h"
#include "bina/document/DocumentFactory.h"
#include "bina/document/FieldTree.h"

namespace bina::app {

DocumentService::DocumentService(document::Document &document, MessageLog &log, QObject *parent)
    : QObject{parent}
    , m_document{document}
    , m_log{log}
{
    // Qualquer alteração da estrutura, das opções, dos tipos ou dos formatos deixa o documento com
    // alterações não salvas. Os tipos são editados em um rascunho: só a definição aplicada conta.
    const auto markDirty = [this] { setDirty(true); };

    const document::FieldTree *tree = m_document.fields();
    connect(tree, &document::FieldTree::inserted, this, markDirty);
    connect(tree, &document::FieldTree::removed, this, markDirty);
    connect(tree, &document::FieldTree::moved, this, markDirty);
    connect(tree, &document::FieldTree::fieldChanged, this, markDirty);

    connect(&m_document, &document::Document::byteOrderChanged, this, markDirty);
    connect(&m_document, &document::Document::maxAlignChanged, this, markDirty);
    connect(&m_document, &document::Document::formatsChanged, this, markDirty);
    connect(&m_document, &document::Document::formatAdded, this, markDirty);
    connect(&m_document, &document::Document::typeAdded, this, markDirty);
    connect(&m_document, &document::Document::typeDefinitionChanged, this, markDirty);
}

bool DocumentService::newDocument()
{
    if (!load(document::DocumentDto(), QString()))
        return false;

    m_log.add(tr("Nova estrutura criada."));

    return true;
}

bool DocumentService::open(const QString &pathOrUrl)
{
    const QString path = localPath(pathOrUrl);
    const QString name = QFileInfo(path).fileName();
    const io::DocumentCodec *codec = m_codecs.forPath(path);

    if (!codec || !codec->importer) {
        m_log.add(tr("Não foi possível abrir '%1': formato desconhecido.").arg(path), true);
        return false;
    }

    const std::unique_ptr<io::DocumentImporter> importer = codec->importer();
    document::DocumentDto dto;

    if (const schema::OperationResult read = importer->readFile(path, dto); !read) {
        m_log.add(tr("Não foi possível abrir '%1': %2").arg(path, read.error()), true);
        return false;
    }

    for (const QString &warning : importer->warnings())
        m_log.add(name + QStringLiteral(": ") + warning);

    const bool resource = isResourcePath(path);

    if (!load(dto, resource ? QString() : path))
        return false;

    m_log.add(resource ? tr("Exemplo carregado: %1").arg(name) : tr("Estrutura carregada: %1").arg(name));

    return true;
}

bool DocumentService::save()
{
    return !m_path.isEmpty() && saveAs(m_path);
}

bool DocumentService::saveAs(const QString &pathOrUrl)
{
    QString path;

    if (const schema::OperationResult resolved = resolveSavePath(pathOrUrl, m_codecs.defaultSuffix(), path); !resolved) {
        m_log.add(tr("Não foi possível salvar: %1").arg(resolved.error()), true);
        return false;
    }

    const io::DocumentCodec *codec = m_codecs.forPath(path);

    if (!codec || !codec->exporter) {
        m_log.add(tr("Não foi possível salvar '%1': formato desconhecido.").arg(path), true);
        return false;
    }

    if (const schema::OperationResult written = codec->exporter()->writeFile(path, document::DocumentFactory::toDto(m_document));
        !written) {
        m_log.add(tr("Não foi possível salvar '%1': %2").arg(path, written.error()), true);
        return false;
    }

    setPath(path);
    setDirty(false);
    m_log.add(tr("Estrutura salva: %1").arg(QFileInfo(path).fileName()));

    return true;
}

// Carrega a estrutura no documento (atômico) e zera o estado de alterações.
bool DocumentService::load(const document::DocumentDto &dto, const QString &path)
{
    document::DocumentFactory factory;

    if (const schema::OperationResult created = factory.create(dto, m_document); !created) {
        m_log.add(tr("Estrutura inválida: %1").arg(created.error()), true);
        return false;
    }

    setPath(path);
    setDirty(false);

    return true;
}

void DocumentService::setDirty(bool dirty)
{
    if (m_dirty == dirty)
        return;

    m_dirty = dirty;
    emit dirtyChanged();
}

void DocumentService::setPath(const QString &path)
{
    if (m_path == path)
        return;

    m_path = path;
    emit pathChanged();
}

} // namespace bina::app

#include "bina/presentation/TypeDefinitionController.h"

#include "bina/document/Document.h"
#include "bina/document/FieldTree.h"
#include "bina/schema/DataType.h"
#include "bina/schema/Field.h"
#include "bina/schema/Identifier.h"

namespace bina::presentation {

TypeDefinitionController::TypeDefinitionController(Kind kind, QObject *parent)
    : QObject{parent}
    , m_kind{kind}
    , m_draft{new document::FieldTree(this)}
    , m_draftModel{new model::SchemaTreeModel(m_draft, this)}
{
    connect(this, &TypeDefinitionController::typeIdsChanged, this, &TypeDefinitionController::choicesChanged);
    connect(this, &TypeDefinitionController::libraryIdsChanged, this, &TypeDefinitionController::choicesChanged);

    // Qualquer edição do rascunho (os auto_pad que a acompanham também) o marca como alterado.
    const auto markModified = [this] {
        setModified(true);
        emit sizeChanged();
    };

    connect(m_draft, &document::FieldTree::inserted, this, markModified);
    connect(m_draft, &document::FieldTree::removed, this, markModified);
    connect(m_draft, &document::FieldTree::moved, this, markModified);
    connect(m_draft, &document::FieldTree::fieldChanged, this, markModified);
}

void TypeDefinitionController::setDocument(document::Document *document)
{
    if (m_document == document)
        return;

    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);

    m_document = document;

    if (m_document) {
        // O registro de tipos (e as definições) é substituído ao carregar uma estrutura ou aplicar
        // um tipo: o rascunho precisa ser refeito depois.
        connect(m_document, &document::Document::typesChanged, this, &TypeDefinitionController::rebuild);

        // Um tipo novo (extraído da estrutura ou copiado da biblioteca) não muda os existentes: só a
        // lista é refeita, e as edições do rascunho continuam.
        const auto added = [this] {
            const QString oldCurrent = m_current;

            if (refreshTypeIds() && m_current != oldCurrent) {
                loadDraft();
                emit currentTypeChanged();
            }

            updateLibraryState();
        };

        connect(m_document, &document::Document::typeAdded, this, added);
        connect(m_document, &document::Document::typeImported, this, added);
        connect(m_document, &document::Document::typeDefinitionChanged, this, &TypeDefinitionController::updateLibraryState);
        connect(m_document, &document::Document::libraryChanged, this, &TypeDefinitionController::updateLibraryState);
        connect(m_document, &document::Document::libraryChanged, this, &TypeDefinitionController::libraryIdsChanged);
        connect(m_document, &document::Document::typeImported, this, &TypeDefinitionController::libraryIdsChanged);
        connect(m_document, &document::Document::activeHeaderChanged, this, &TypeDefinitionController::activeChanged);

        // Os tamanhos e os auto_pad dependem do alinhamento máximo.
        connect(m_document, &document::Document::maxAlignChanged, this, [this] {
            const bool wasModified = m_modified;

            m_draft->setMaxAlign(m_document->maxAlign());
            m_draft->resyncPadding();

            setModified(wasModified);
        });
    }

    rebuild();
}

void TypeDefinitionController::setCurrentType(const QString &id)
{
    if (id == m_current || !m_typeIds.contains(id))
        return;

    m_current = id;
    loadDraft();

    emit currentTypeChanged();
    emit activeChanged();
    updateLibraryState();
}

bool TypeDefinitionController::isActive() const
{
    return m_kind == Headers && m_document && !m_current.isEmpty() && m_document->activeHeader() == m_current;
}

void TypeDefinitionController::setActive(bool active)
{
    if (m_kind != Headers || !m_document || active == isActive())
        return;

    if (active && m_current.isEmpty())
        return;

    m_document->setActiveHeader(active ? m_current : QString());
}

QStringList TypeDefinitionController::libraryIds() const
{
    if (!m_document)
        return {};

    return m_kind == Headers ? m_document->availableLibraryHeaderIds() : m_document->availableLibraryTypeIds();
}

QVariantList TypeDefinitionController::choices() const
{
    QVariantList list;

    for (const QString &id : m_typeIds)
        list << QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("text"), id}, {QStringLiteral("fromLibrary"), false}};

    for (const QString &id : libraryIds()) {
        list << QVariantMap{{QStringLiteral("id"), id},
                            {QStringLiteral("text"), tr("%1 (biblioteca)").arg(id)},
                            {QStringLiteral("fromLibrary"), true}};
    }

    return list;
}

bool TypeDefinitionController::select(const QString &id)
{
    if (m_typeIds.contains(id)) {
        setCurrentType(id);
        return true;
    }

    return importFromLibrary(id);
}

int TypeDefinitionController::sizeInBytes() const
{
    return m_draft->root() ? m_draft->sizeInBytes(m_draft->root()) : 0;
}

bool TypeDefinitionController::importFromLibrary(const QString &id)
{
    if (!m_document)
        return false;

    const schema::OperationResult result = m_document->importType(id);

    if (!result) {
        emit errorOccurred(result.error());
        return false;
    }

    setCurrentType(schema::normalizeIdentifier(id));

    return true;
}

bool TypeDefinitionController::apply()
{
    if (!m_document || m_current.isEmpty() || !m_draft->root())
        return false;

    // A definição do tipo não guarda auto_pad: o rascunho os recria ao ser carregado.
    const schema::OperationResult result = m_document->setTypeDefinition(m_current, m_draft->root()->clone(false));

    if (!result) {
        emit errorOccurred(result.error());
        return false;
    }

    // typesChanged() já recarregou o rascunho a partir da definição aplicada.
    return true;
}

bool TypeDefinitionController::createType(const QString &id)
{
    if (!m_document)
        return false;

    const schema::OperationResult result = m_kind == Headers ? m_document->addHeader(id) : m_document->addType(id);

    if (!result) {
        emit errorOccurred(result.error());
        return false;
    }

    // typesChanged() refez a lista; falta apontar para o tipo novo.
    setCurrentType(schema::normalizeIdentifier(id));

    return true;
}

void TypeDefinitionController::restore()
{
    loadDraft();
}

bool TypeDefinitionController::removeType()
{
    if (!m_document || m_current.isEmpty())
        return false;

    // typesChanged() refaz a lista e escolhe outro tipo.
    const schema::OperationResult result = m_document->removeType(m_current);

    if (!result) {
        emit errorOccurred(result.error());
        return false;
    }

    return true;
}

bool TypeDefinitionController::updateFromLibrary()
{
    if (!m_document || m_current.isEmpty())
        return false;

    const schema::OperationResult result = m_document->updateTypeFromLibrary(m_current);

    if (!result) {
        emit errorOccurred(result.error());
        return false;
    }

    return true;
}

// Refaz a lista de tipos customizados, mantendo o tipo atual se ele ainda existe.
void TypeDefinitionController::rebuild()
{
    const QString oldCurrent = m_current;

    refreshTypeIds();
    loadDraft();

    if (m_current != oldCurrent)
        emit currentTypeChanged();

    emit activeChanged();
    emit libraryIdsChanged();
    updateLibraryState();
}

// A lista de tipos customizados; o tipo atual continua se ainda existe. Retorna se a lista mudou.
bool TypeDefinitionController::refreshTypeIds()
{
    const QStringList oldIds = m_typeIds;

    m_typeIds.clear();

    if (m_document) {
        for (const schema::DataType *type : m_document->types()->types()) {
            if (type->definition() && m_document->isHeader(type->id()) == (m_kind == Headers))
                m_typeIds << type->id();
        }
    }

    m_current = m_typeIds.contains(m_current) ? m_current : m_typeIds.value(0);

    if (m_typeIds == oldIds)
        return false;

    emit typeIdsChanged();

    return true;
}

void TypeDefinitionController::updateLibraryState()
{
    const LibraryState state = m_document && !m_current.isEmpty()
                                   ? LibraryState(m_document->libraryState(m_current))
                                   : NotInLibrary;

    if (state == m_libraryState)
        return;

    m_libraryState = state;
    emit libraryStateChanged();
}

// O rascunho recebe uma cópia da definição do tipo atual (ou nada, se não há tipos).
void TypeDefinitionController::loadDraft()
{
    m_draft->setRegistry(m_document ? m_document->types() : nullptr);
    m_draft->setFormatRegistry(m_document ? m_document->formats() : nullptr);
    m_draft->setMaxAlign(m_document ? m_document->maxAlign() : 4);
    m_draft->setEditedType(m_current);

    const schema::Field *definition = m_document && !m_current.isEmpty()
                                            ? m_document->types()->definition(m_current) : nullptr;

    m_draft->setRoot(definition ? definition->clone() : nullptr);

    setModified(false);
    emit sizeChanged();
}

void TypeDefinitionController::setModified(bool modified)
{
    if (m_modified == modified)
        return;

    m_modified = modified;
    emit modifiedChanged();
}

} // namespace bina::presentation

#include "bina/presentation/TypeDefinitionController.h"

#include "bina/document/Document.h"
#include "bina/document/FieldTree.h"
#include "bina/schema/DataType.h"
#include "bina/schema/Field.h"
#include "bina/schema/Identifier.h"

namespace bina::presentation {

TypeDefinitionController::TypeDefinitionController(QObject *parent)
    : QObject{parent}
    , m_draft{new document::FieldTree(this)}
    , m_draftModel{new model::SchemaTreeModel(m_draft, this)}
{
    // Qualquer edição do rascunho (os auto_pad que a acompanham também) o marca como alterado.
    const auto markModified = [this] { setModified(true); };

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

    const schema::OperationResult result = m_document->addType(id);

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

// Refaz a lista de tipos customizados, mantendo o tipo atual se ele ainda existe.
void TypeDefinitionController::rebuild()
{
    const QStringList oldIds = m_typeIds;
    const QString oldCurrent = m_current;

    m_typeIds.clear();

    if (m_document) {
        for (const schema::DataType *type : m_document->types()->types()) {
            if (type->definition())
                m_typeIds << type->id();
        }
    }

    m_current = m_typeIds.contains(m_current) ? m_current : m_typeIds.value(0);

    loadDraft();

    if (m_typeIds != oldIds)
        emit typeIdsChanged();

    if (m_current != oldCurrent)
        emit currentTypeChanged();
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
}

void TypeDefinitionController::setModified(bool modified)
{
    if (m_modified == modified)
        return;

    m_modified = modified;
    emit modifiedChanged();
}

} // namespace bina::presentation

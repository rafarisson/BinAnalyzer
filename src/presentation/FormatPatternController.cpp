#include "bina/presentation/FormatPatternController.h"

#include "bina/document/Document.h"
#include "bina/format/FormatRegistry.h"
#include "bina/format/PatternPreview.h"

namespace bina::presentation {

FormatPatternController::FormatPatternController(QObject *parent)
    : QObject{parent}
{
}

void FormatPatternController::setDocument(document::Document *document)
{
    if (m_document == document)
        return;

    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);

    m_document = document;

    if (m_document) {
        // O registro de formatos é substituído junto com o de tipos ao carregar uma estrutura.
        connect(m_document, &document::Document::typesChanged, this, &FormatPatternController::rebuild);
        connect(m_document, &document::Document::formatAdded, this, &FormatPatternController::rebuild);
    }

    rebuild();
}

void FormatPatternController::setCurrentPattern(const QString &id)
{
    if (id == m_current || !m_ids.contains(id))
        return;

    m_current = id;
    m_spec = currentSpec();
    emit currentPatternChanged();
    emit currentSpecChanged();
}

bool FormatPatternController::applyPattern(const QString &spec)
{
    if (!m_document || m_current.isEmpty())
        return false;

    if (const schema::OperationResult changed = m_document->setFormatPattern(m_current, spec); !changed) {
        emit errorOccurred(changed.error());
        return false;
    }

    m_spec = spec;
    emit currentSpecChanged();

    return true;
}

bool FormatPatternController::createPattern(const QString &id, const QString &spec)
{
    if (!m_document)
        return false;

    const QString name = id.trimmed();

    if (const schema::OperationResult added = m_document->addFormat(name, spec); !added) {
        emit errorOccurred(added.error());
        return false;
    }

    // formatAdded() já refez a lista; falta apontar para o formato novo.
    setCurrentPattern(name);

    return true;
}

QString FormatPatternController::currentSpec() const
{
    if (!m_document || m_current.isEmpty())
        return {};

    for (const format::FormatRegistry::Pattern &pattern : m_document->formats()->patterns()) {
        if (pattern.id == m_current)
            return pattern.spec;
    }

    return {};
}

QString FormatPatternController::suggestSample(const QString &spec) const
{
    return format::suggestSample(spec);
}

QVariantMap FormatPatternController::preview(const QString &spec, const QString &sample) const
{
    const format::PatternPreviewResult result = format::previewPattern(spec, sample);

    return {
        {QStringLiteral("patternValid"), result.patternValid},
        {QStringLiteral("ok"), result.ok},
        {QStringLiteral("text"), result.text},
        {QStringLiteral("error"), result.error}
    };
}

// Refaz a lista de formatos, mantendo o atual se ele ainda existe.
void FormatPatternController::rebuild()
{
    const QStringList oldIds = m_ids;
    const QString oldCurrent = m_current;
    const QString oldSpec = m_spec;

    m_ids.clear();

    if (m_document) {
        for (const format::FormatRegistry::Pattern &pattern : m_document->formats()->patterns())
            m_ids << pattern.id;
    }

    m_current = m_ids.contains(m_current) ? m_current : m_ids.value(0);
    m_spec = currentSpec();

    // Só avisa o que mudou: trocar a definição de um tipo não deve apagar o teste em andamento.
    if (m_ids != oldIds)
        emit patternIdsChanged();

    if (m_current != oldCurrent || m_spec != oldSpec)
        emit currentPatternChanged();

    if (m_spec != oldSpec)
        emit currentSpecChanged();
}

} // namespace bina::presentation

#include <QCoreApplication>

#include "bina/document/Document.h"
#include "bina/document/FieldTree.h"
#include "bina/schema/Field.h"
#include "bina/schema/Identifier.h"
#include "bina/schema/LayoutCalculator.h"
#include "bina/schema/Builtins.h"

namespace bina::document {

Document::Document(QObject *parent)
    : QObject{parent}
    , m_registry{schema::TypeRegistry::withBuiltins()}
    , m_formats{std::make_unique<format::FormatRegistry>()}
{
    m_fields = std::make_unique<FieldTree>();
    m_fields->setRegistry(m_registry.get());
    m_fields->setMaxAlign(m_maxAlign);
    m_fields->setFormatRegistry(m_formats.get());
}

Document::~Document() = default;

void Document::setByteOrder(QSysInfo::Endian order)
{
    if (m_byteOrder == order)
        return;

    m_byteOrder = order;
    emit byteOrderChanged();
}

bool Document::setMaxAlign(int align)
{
    if (align == m_maxAlign)
        return true;

    if (!schema::Alignment::validateMax(align))
        return false;

    m_maxAlign = align;
    m_fields->setMaxAlign(align);
    m_fields->resyncPadding();
    emit maxAlignChanged();

    return true;
}

schema::OperationResult Document::setFormatPattern(const QString &id, const QString &spec)
{
    for (const format::FormatRegistry::Pattern &pattern : m_formats->patterns()) {
        if (pattern.id == id && pattern.spec == spec)
            return schema::OperationResult::success();
    }

    const schema::OperationResult changed = m_formats->setPattern(id, spec);

    if (changed)
        emit formatsChanged();

    return changed;
}

schema::OperationResult Document::setTypeDefinition(const QString &id, std::unique_ptr<schema::Field> definition)
{
    if (const schema::OperationResult checked = m_registry->checkReplacement(id, definition.get()); !checked)
        return checked;

    // Quem guarda ponteiros para a definição antiga (models, resultado da análise) os solta agora.
    emit typesAboutToChange();

    const schema::OperationResult replaced = m_registry->replaceDefinition(id, std::move(definition));

    // O layout do tipo mudou: os auto_pad da estrutura dependem dele.
    m_fields->resyncPadding();

    emit typesChanged();
    emit typeDefinitionChanged(id);

    return replaced;
}

schema::OperationResult Document::addType(const QString &id)
{
    auto definition = std::make_unique<schema::Field>();
    schema::Field *first = definition->insertChild(0, QStringLiteral("%1_1").arg(schema::Names::kNewField));

    if (!first || first->setTypeId(schema::PrimitiveTypes::kUInt8) == schema::Field::Edit::Invalid)
        return schema::OperationResult::failure(QCoreApplication::translate("bina", "Não foi possível criar o primeiro campo"));

    const schema::OperationResult added = m_registry->add(id, std::move(definition));

    if (!added)
        return added;

    const QString normalized = schema::normalizeIdentifier(id);

    emit typesChanged();
    emit typeAdded(normalized);

    return added;
}

schema::OperationResult Document::addFormat(const QString &id, const QString &spec)
{
    const schema::OperationResult added = m_formats->addPattern(id, spec);

    if (added)
        emit formatAdded(id);

    return added;
}

} // namespace bina::document

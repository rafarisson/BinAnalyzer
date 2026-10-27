#include <QCoreApplication>

#include "bina/document/Document.h"
#include "bina/document/DocumentFactory.h"
#include "bina/document/FieldTree.h"
#include "bina/document/TypeCatalog.h"
#include "bina/schema/DataType.h"
#include "bina/schema/Field.h"
#include "bina/schema/Identifier.h"
#include "bina/schema/LayoutCalculator.h"
#include "bina/schema/Builtins.h"

namespace bina::document {

namespace {

// Os filhos de 'a' e 'b' (o próprio campo não conta: é a raiz de uma definição).
bool sameFields(const DocumentFieldDto &a, const DocumentFieldDto &b)
{
    if (a.children.size() != b.children.size())
        return false;

    for (qsizetype i = 0; i < a.children.size(); ++i) {
        const DocumentFieldDto &x = a.children.at(i);
        const DocumentFieldDto &y = b.children.at(i);

        if (x.name != y.name || x.type != y.type || x.formatter != y.formatter || x.count != y.count || !sameFields(x, y))
            return false;
    }

    return true;
}

} // namespace

Document::Document(QObject *parent)
    : QObject{parent}
    , m_registry{schema::TypeRegistry::withBuiltins()}
    , m_formats{std::make_unique<format::FormatRegistry>()}
{
    m_fields = std::make_unique<FieldTree>();
    m_fields->setRegistry(m_registry.get());
    m_fields->setMaxAlign(m_maxAlign);
    m_fields->setFormatRegistry(m_formats.get());
    m_fields->setTypeSource(&m_typeSource);
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

schema::OperationResult Document::removeType(const QString &id)
{
    using schema::OperationResult;

    if (m_fields->root() && m_registry->definition(id) && m_registry->usesType(m_fields->root(), id))
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' é usado pela estrutura").arg(id));

    // Nenhum campo usa o tipo: o resultado da análise não guarda ponteiros para ele.
    if (const OperationResult removed = m_registry->remove(id); !removed)
        return removed;

    emit typesChanged();
    emit typeRemoved(id);

    return OperationResult::success();
}

void Document::setLibrary(const TypeCatalog *library)
{
    m_library = library;
    emit libraryChanged();
}

QStringList LibraryTypeSource::availableTypeIds() const
{
    return m_document.availableLibraryTypeIds();
}

schema::OperationResult LibraryTypeSource::provideType(const QString &id)
{
    return m_document.importType(id);
}

QStringList Document::availableLibraryTypeIds() const
{
    QStringList ids;

    if (m_library) {
        for (const QString &id : m_library->typeIds())
            if (!m_registry->contains(id))
                ids << id;
    }

    return ids;
}

schema::OperationResult Document::extractType(schema::Field *group, const QString &id)
{
    using schema::OperationResult;

    if (!m_fields->canCollapseToType(group))
        return OperationResult::failure(QCoreApplication::translate("bina", "Só um grupo com campos pode virar um tipo"));

    // A definição de um tipo não guarda auto_pad: o layout os recalcula igual.
    auto definition = std::make_unique<schema::Field>();

    for (int i = 0, row = 0; i < group->childCount(); ++i) {
        if (!group->child(i)->isAutoPad())
            definition->insertExistingChild(row++, group->child(i)->clone(false));
    }

    if (const OperationResult added = m_registry->add(id, std::move(definition)); !added)
        return added;

    const QString normalized = schema::normalizeIdentifier(id);

    emit typeAdded(normalized);

    m_fields->collapseToType(group, normalized);

    return OperationResult::success();
}

schema::OperationResult Document::importType(const QString &id)
{
    ImportPlan plan;
    QStringList visiting;

    if (const schema::OperationResult planned = planType(id, plan, visiting); !planned)
        return planned;

    return apply(plan);
}

Document::LibraryState Document::libraryState(const QString &id) const
{
    const schema::DataType *type = m_registry->find(id);
    const DocumentTypeDto *entry = m_library ? m_library->type(id) : nullptr;

    if (!type || !type->definition() || !entry)
        return LibraryState::NotInLibrary;

    // Pelo mesmo caminho do documento, para que a forma do JSON (grupos sem "type", nomes não
    // normalizados) não conte como diferença.
    const DocumentFieldDto library = DocumentFactory::toFieldDto(*DocumentFactory::buildField(entry->definition));
    const DocumentFieldDto current = DocumentFactory::toFieldDto(*type->definition());
    const bool same = sameFields(library, current) && entry->defaultFormatter.trimmed() == type->formatId();

    return same ? LibraryState::SameAsLibrary : LibraryState::DiffersFromLibrary;
}

schema::OperationResult Document::updateTypeFromLibrary(const QString &id)
{
    using schema::OperationResult;

    const DocumentTypeDto *entry = m_library ? m_library->type(id) : nullptr;

    if (!entry || !m_registry->definition(id))
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' não está na biblioteca").arg(id));

    // Só as dependências: o próprio tipo já está no documento.
    ImportPlan plan;
    QStringList visiting{id};

    if (const OperationResult planned = planFields(entry->definition, plan, visiting); !planned)
        return planned;

    if (const OperationResult applied = apply(plan); !applied)
        return applied;

    return setTypeDefinition(id, DocumentFactory::buildField(entry->definition));
}

schema::OperationResult Document::planType(const QString &id, ImportPlan &plan, QStringList &visiting) const
{
    using schema::OperationResult;

    if (m_registry->contains(id) || plan.types.contains(id))
        return OperationResult::success();

    const DocumentTypeDto *entry = m_library ? m_library->type(id) : nullptr;

    if (!entry) {
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' não existe no documento nem na biblioteca")
                                            .arg(id));
    }

    if (visiting.contains(id))
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' da biblioteca contém a si mesmo").arg(id));

    visiting << id;

    if (const OperationResult format = planFormat(entry->defaultFormatter, plan); !format)
        return format;

    if (const OperationResult fields = planFields(entry->definition, plan, visiting); !fields)
        return fields;

    visiting.removeLast();

    // Depois das dependências: o registro só aceita campos de tipos já registrados.
    plan.types << id;

    return OperationResult::success();
}

schema::OperationResult Document::planFields(const DocumentFieldDto &parent, ImportPlan &plan, QStringList &visiting) const
{
    using schema::OperationResult;

    for (const DocumentFieldDto &child : parent.children) {
        if (const OperationResult format = planFormat(child.formatter, plan); !format)
            return format;

        const OperationResult nested = child.children.isEmpty()
                                           ? planType(schema::normalizeIdentifier(child.type), plan, visiting)
                                           : planFields(child, plan, visiting);
        if (!nested)
            return nested;
    }

    return OperationResult::success();
}

schema::OperationResult Document::planFormat(const QString &id, ImportPlan &plan) const
{
    using schema::OperationResult;

    const QString trimmed = id.trimmed();

    // Os embutidos, os padrões inline e os do documento já existem; vale o do documento.
    if (trimmed.isEmpty() || m_formats->contains(trimmed) || plan.formats.contains(trimmed))
        return OperationResult::success();

    if (!m_library || !m_library->formatter(trimmed)) {
        return OperationResult::failure(QCoreApplication::translate("bina", "O formato '%1' não existe no documento nem na biblioteca")
                                            .arg(trimmed));
    }

    plan.formats << trimmed;

    return OperationResult::success();
}

// O plano já conferiu que tudo existe; a biblioteca só guarda tipos validados ao serem lidos.
schema::OperationResult Document::apply(const ImportPlan &plan)
{
    using schema::OperationResult;

    for (const QString &id : plan.formats) {
        if (const OperationResult added = m_formats->addPattern(id, m_library->formatter(id)->pattern); !added)
            return added;

        emit formatAdded(id);
    }

    for (const QString &id : plan.types) {
        const DocumentTypeDto *entry = m_library->type(id);

        if (const OperationResult added = m_registry->add(id, DocumentFactory::buildField(entry->definition), entry->defaultFormatter);
            !added) {
            return added;
        }

        emit typeImported(id);
    }

    return OperationResult::success();
}

} // namespace bina::document

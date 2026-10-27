#include "bina/document/FieldTree.h"

// bina::schema
#include "bina/schema/DataType.h"
#include "bina/schema/Identifier.h"
#include "bina/schema/LayoutCalculator.h"
#include "bina/schema/Builtins.h"
#include "bina/schema/TypeRegistry.h"

// bina::format
#include "bina/format/FieldFormat.h"
#include "bina/format/FormatRegistry.h"

namespace bina::document {

FieldTree::FieldTree(QObject *parent)
    : QObject{parent}
{
}

void FieldTree::setRoot(std::unique_ptr<schema::Field> field)
{
    touch();
    emit aboutToReset();

    {
        QSignalBlocker blocker(this);

        m_root = std::move(field);

        if (m_root) {
            m_root->setTypeId(schema::PrimitiveTypes::kStruct);
            normalize(m_root.get());
        }
    }

    emit reset();
}

bool FieldTree::usesEditedType(const QString &typeId) const
{
    return !m_editedType.isEmpty() && m_registry && m_registry->dependsOn(typeId, m_editedType);
}

QStringList FieldTree::typeIds() const
{
    QStringList ids;

    if (m_registry) {
        for (const schema::DataType *type : m_registry->types())
            if (type->id() != schema::PrimitiveTypes::kStruct && !usesEditedType(type->id()))
                ids << type->id();
    }

    return ids;
}

QStringList FieldTree::compatibleFormatIds(const schema::Field *field) const
{
    QStringList ids;

    if (field && m_registry && m_formats) {
        for (const format::Formatter *f : m_formats->compatible(format::targetFor(*field, *m_registry)))
            ids << f->id();
    }

    return ids;
}

int FieldTree::sizeInBytes(const schema::Field *field) const
{
    return field ? schema::LayoutCalculator(m_registry, m_maxAlign).sizeOf(field) : 0;
}

bool FieldTree::setName(schema::Field *field, const QString &name)
{
    if (!field || field->isAutoPad())
        return false;

    const QString normalized = schema::normalizeIdentifier(name);

    if (normalized == field->name())
        return false;

    if (normalized.isEmpty()) {
        reject(tr("O nome do campo não pode ser vazio."));
        return false;
    }

    if (hasSiblingNamed(field, normalized)) {
        reject(tr("Já existe um campo chamado '%1' neste grupo.").arg(normalized));
        return false;
    }

    if (field->setName(normalized) != schema::Field::Edit::Changed)
        return false;

    emit fieldChanged(field);

    return true;
}

bool FieldTree::setTypeId(schema::Field *field, const QString &id)
{
    if (!field || field == m_root.get() || field->isAutoPad())
        return false;

    const QString normalized = schema::normalizeIdentifier(id);
    const bool isStruct = normalized == schema::PrimitiveTypes::kStruct;

    // O tipo struct é implícito: existe se e somente se o campo tem filhos.
    if (isStruct != (field->childCount() > 0)) {
        reject(isStruct ? tr("O tipo 'struct' é definido pelos campos filhos.")
                        : tr("Um grupo com campos não pode mudar de tipo."));
        return false;
    }

    if (m_registry && !m_registry->contains(normalized)) {
        reject(tr("Tipo desconhecido: '%1'.").arg(normalized));
        return false;
    }

    if (usesEditedType(normalized)) {
        reject(tr("O tipo '%1' não pode ser usado: a definição conteria a si mesma.").arg(normalized));
        return false;
    }

    if (field->setTypeId(normalized) != schema::Field::Edit::Changed)
        return false;

    dropIncompatibleFormat(field);
    updatePadding(field->parent());
    emit fieldChanged(field);

    return true;
}

bool FieldTree::setCount(schema::Field *field, int count)
{
    if (!field || field->isAutoPad())
        return false;

    if (count < 1 || count > schema::Field::kMaxCount) {
        reject(tr("A quantidade deve estar entre 1 e %1.").arg(schema::Field::kMaxCount));
        return false;
    }

    if (field->setCount(count) != schema::Field::Edit::Changed)
        return false;

    dropIncompatibleFormat(field);
    updatePadding(field->parent());
    emit fieldChanged(field);

    return true;
}

bool FieldTree::setFormatId(schema::Field *field, const QString &id)
{
    if (!field || field == m_root.get() || field->isAutoPad())
        return false;

    // Vazio volta ao formato padrão do tipo.
    const QString trimmed = id.trimmed();

    if (!trimmed.isEmpty() && m_registry && m_formats
        && !format::isFormatValid(*field, *m_registry, *m_formats, trimmed)) {
        reject(tr("O formato '%1' não é compatível com este campo.").arg(trimmed));
        return false;
    }

    if (field->setFormatId(trimmed) != schema::Field::Edit::Changed)
        return false;

    emit fieldChanged(field);

    return true;
}

bool FieldTree::hasSiblingNamed(const schema::Field *field, const QString &name) const
{
    const schema::Field *parent = field->parent();

    for (int i = 0; parent && i < parent->childCount(); ++i) {
        const schema::Field *sibling = parent->child(i);

        if (sibling != field && !sibling->isAutoPad() && sibling->name() == name)
            return true;
    }

    return false;
}

// Ao mudar o tipo ou a quantidade, um formato que não serve mais ao campo volta ao padrão.
void FieldTree::dropIncompatibleFormat(schema::Field *field)
{
    if (!m_registry || !m_formats || field->formatId().isEmpty())
        return;

    if (!format::isFormatValid(*field, *m_registry, *m_formats, field->formatId()))
        field->setFormatId(QString());
}

// Um campo que ganha filhos passa a ser uma struct (a raiz já é).
void FieldTree::becomeStruct(schema::Field *parent)
{
    if (parent != m_root.get() && parent->setTypeId(schema::PrimitiveTypes::kStruct) == schema::Field::Edit::Changed) {
        dropIncompatibleFormat(parent);
        emit fieldChanged(parent);
    }
}

void FieldTree::reject(const QString &message)
{
    emit editRejected(message);
}

} // namespace bina::document

#include <QCoreApplication>
#include <QSet>

#include "bina/schema/TypeRegistry.h"
#include "bina/schema/Identifier.h"
#include "bina/schema/Builtins.h"
#include "bina/schema/DataType.h"
#include "bina/schema/Types.h"
#include "bina/schema/LayoutCalculator.h"

namespace bina::schema {

TypeRegistry::TypeRegistry() = default;

TypeRegistry::~TypeRegistry()
{
    qDeleteAll(m_types);
}

std::unique_ptr<TypeRegistry> TypeRegistry::withBuiltins()
{
    auto registry = std::make_unique<TypeRegistry>();

    registry->add(PrimitiveTypes::kChar,      1,  1, Representation::Char);
    registry->add(PrimitiveTypes::kInt8,      1,  1, Representation::Signed);
    registry->add(PrimitiveTypes::kUInt8,     1,  1, Representation::Unsigned);
    registry->add(PrimitiveTypes::kInt16,     2,  2, Representation::Signed);
    registry->add(PrimitiveTypes::kUInt16,    2,  2, Representation::Unsigned);
    registry->add(PrimitiveTypes::kInt32,     4,  4, Representation::Signed);
    registry->add(PrimitiveTypes::kUInt32,    4,  4, Representation::Unsigned);
    registry->add(PrimitiveTypes::kEnum,      4,  4, Representation::Enum);
    registry->add(PrimitiveTypes::kFloat,     4,  4, Representation::Float);
    registry->add(PrimitiveTypes::kVoid,      4,  4, Representation::Opaque);
    registry->add(PrimitiveTypes::kInt64,     8,  8, Representation::Signed);
    registry->add(PrimitiveTypes::kUInt64,    8,  8, Representation::Unsigned);
    registry->add(PrimitiveTypes::kDouble,    8,  8, Representation::Float);

    registry->addStructMarker(PrimitiveTypes::kStruct);

    return registry;
}

QList<const DataType *> TypeRegistry::types() const
{
    QList<const DataType *> out;
    for (const QString &id : m_order)
        out << m_types.value(id);
    return out;
}

const Field *TypeRegistry::definition(const QString &id) const
{
    const DataType *type = m_types.value(id);
    return type ? type->definition() : nullptr;
}

OperationResult TypeRegistry::add(const QString &id, int size, int align, Representation representation)
{
    QString normalized;
    if (const OperationResult checked = checkNewId(id, normalized); !checked)
        return checked;

    if (size <= 0)
        return OperationResult::failure(QCoreApplication::translate("bina", "Tamanho inválido"));

    if (!Alignment::isPowerOfTwo(align))
        return OperationResult::failure(QCoreApplication::translate("bina", "O alinhamento de '%1' deve ser uma potência de 2").arg(normalized));

    insert(DataType::makePrimitive(normalized, size, align, representation));

    return OperationResult::success();
}

OperationResult TypeRegistry::add(const QString &id, std::unique_ptr<Field> definition, const QString &formatId)
{
    if (!definition)
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' não tem definição").arg(id));

    QString normalized;
    if (const OperationResult checked = checkNewId(id, normalized); !checked)
        return checked;

    if (const OperationResult valid = validateDefinition(normalized, definition.get()); !valid)
        return valid;

    // A validade do layout não depende do alinhamento máximo: só de ter ao menos um campo válido.
    if (LayoutCalculator(this, Alignment::kDefaultMax).structLayout(definition.get()).size <= 0)
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' deve ter pelo menos um campo").arg(normalized));

    insert(DataType::makeStruct(normalized, std::move(definition), formatId.trimmed()));

    return OperationResult::success();
}

bool TypeRegistry::dependsOn(const QString &typeId, const QString &target) const
{
    QSet<QString> checked;
    return dependsOn(typeId, target, checked);
}

bool TypeRegistry::dependsOn(const QString &typeId, const QString &target, QSet<QString> &checked) const
{
    if (typeId == target)
        return true;

    if (checked.contains(typeId))
        return false;

    const DataType *type = find(typeId);
    const bool uses = type && type->definition() && usesType(type->definition(), target, checked);

    if (!uses)
        checked.insert(typeId);

    return uses;
}

bool TypeRegistry::usesType(const Field *field, const QString &target) const
{
    QSet<QString> checked;
    return usesType(field, target, checked);
}

bool TypeRegistry::usesType(const Field *field, const QString &target, QSet<QString> &checked) const
{
    for (int i = 0; i < field->childCount(); ++i) {
        const Field *child = field->child(i);

        if (child->isAutoPad())
            continue;

        // Um campo com filhos é do tipo "struct": o que vale são os filhos.
        if (dependsOn(child->typeId(), target, checked) || usesType(child, target, checked))
            return true;
    }

    return false;
}

OperationResult TypeRegistry::checkReplacement(const QString &id, const Field *definition) const
{
    const DataType *type = find(id);

    if (!type || !type->definition())
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' não tem definição editável").arg(id));

    if (!definition)
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' não tem definição").arg(id));

    if (const OperationResult valid = validateDefinition(id, definition); !valid)
        return valid;

    if (usesType(definition, id))
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' não pode conter a si mesmo").arg(id));

    if (LayoutCalculator(this, Alignment::kDefaultMax).structLayout(definition).size <= 0)
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' deve ter pelo menos um campo").arg(id));

    return OperationResult::success();
}

OperationResult TypeRegistry::replaceDefinition(const QString &id, std::unique_ptr<Field> definition)
{
    if (const OperationResult checked = checkReplacement(id, definition.get()); !checked)
        return checked;

    DataType *old = m_types.value(id);
    auto replacement = DataType::makeStruct(id, std::move(definition), old->formatId());

    m_types[id] = replacement.release();
    delete old;

    return OperationResult::success();
}

OperationResult TypeRegistry::addStructMarker(const QString &id)
{
    QString normalized;
    if (const OperationResult checked = checkNewId(id, normalized); !checked)
        return checked;

    insert(DataType::makeStruct(normalized));

    return OperationResult::success();
}

OperationResult TypeRegistry::checkNewId(const QString &id, QString &normalized) const
{
    normalized = normalizeIdentifier(id);

    if (normalized.isEmpty())
        return OperationResult::failure(QCoreApplication::translate("bina", "Id de tipo inválido"));

    if (contains(normalized))
        return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' já existe").arg(normalized));

    return OperationResult::success();
}

OperationResult TypeRegistry::validateDefinition(const QString &id, const Field *definition) const
{
    QSet<QString> names;

    for (int i = 0; i < definition->childCount(); ++i) {
        const Field *field = definition->child(i);

        if (field->isAutoPad())
            continue;

        if (names.contains(field->name()))
            return OperationResult::failure(QCoreApplication::translate("bina", "O tipo '%1' tem o campo '%2' repetido").arg(id, field->name()));

        names.insert(field->name());

        // Inclui a auto-referência: o tipo ainda não está registrado.
        if (!contains(field->typeId()))
            return OperationResult::failure(QCoreApplication::translate("bina", "O campo '%1' de '%2' usa o tipo desconhecido '%3'").arg(field->name(), id, field->typeId()));

        if (const OperationResult nested = validateDefinition(id, field); !nested)
            return nested;
    }

    return OperationResult::success();
}

void TypeRegistry::insert(std::unique_ptr<DataType> type)
{
    const QString id = type->id();

    m_types.insert(id, type.release());
    m_order << id;
}

} // namespace bina::schema

#include "bina/model/FieldRef.h"

#include "bina/schema/Field.h"

namespace bina::model {

FieldRef FieldRef::fromNode(const schema::Field *field, const schema::Field *root, quint64 modelToken, quint32 revision)
{
    FieldRef ref;

    if (!field || !root || modelToken == 0)
        return ref;

    for (auto *it = field; it && it != root; it = it->parent()) {
        const int row = it->row();

        if (row < 0)
            return {};

        ref.path.prepend(row);
    }

    // Um campo que não está sob a raiz (ou a própria raiz) não tem referência.
    if (ref.path.isEmpty())
        return {};

    ref.modelToken = modelToken;
    ref.revision = revision;
    ref.fieldId = field->id();

    return ref;
}

schema::Field *FieldRef::toNode(const FieldRef &ref, schema::Field *root, quint64 modelToken)
{
    if (!root || !ref.isValid() || ref.modelToken != modelToken)
        return nullptr;

    schema::Field *node = root;

    for (const int row : ref.path) {
        node = node->child(row);   // devolve nullptr para linhas fora do intervalo

        if (!node)
            return nullptr;
    }

    return node->id() == ref.fieldId ? node : nullptr;
}

} // namespace bina::model

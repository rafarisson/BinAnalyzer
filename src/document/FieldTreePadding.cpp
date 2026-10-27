// Sincronização dos auto_pad (parte de FieldTree).

#include "bina/document/FieldTree.h"
#include "bina/document/PaddingSynchronizer.h"

// bina::schema
#include "bina/schema/Builtins.h"

namespace bina::document {

void FieldTree::updatePadding(schema::Field *from)
{
    for (schema::Field *p = from; p; p = p->parent()) {
        syncPadding(p);
        emit fieldChanged(p);
    }
}

// Aplica o plano do PaddingSynchronizer ao container, avisando as views a cada passo.
void FieldTree::syncPadding(schema::Field *container)
{
    if (!m_registry)
        return;

    const bool hadFields = [container] {
        for (int i = 0; i < container->childCount(); ++i)
            if (!container->child(i)->isAutoPad())
                return true;
        return false;
    }();

    const PaddingSynchronizer synchronizer(m_registry, m_maxAlign);

    for (const PaddingSynchronizer::Step &step : synchronizer.plan(container)) {
        switch (step.kind) {
        case PaddingSynchronizer::Step::Remove:
            touch();
            emit aboutToRemove(container, step.row, step.row);
            container->removeChild(step.row);
            emit removed();
            break;

        case PaddingSynchronizer::Step::Insert:
            touch();
            emit aboutToInsert(container, step.row, step.row);
            container->insertExistingChild(step.row, schema::Field::makeAutoPad(step.size));
            emit inserted();
            break;

        case PaddingSynchronizer::Step::Resize:
            container->child(step.row)->setCount(step.size);
            emit fieldChanged(container->child(step.row));
            break;
        }
    }

    // Sem campos, o container deixa de ser struct. A raiz é sempre struct, mesmo sem campos.
    if (!hadFields && container != m_root.get() && container->typeId() == schema::PrimitiveTypes::kStruct)
        container->setTypeId(schema::PrimitiveTypes::kUInt32);
}

// Pós-ordem, para que o tamanho dos filhos já esteja correto ao calcular o pai.
void FieldTree::normalize(schema::Field *container)
{
    for (int i = 0; i < container->childCount(); ++i) {
        schema::Field *child = container->child(i);
        if (child->childCount() > 0) {
            child->setTypeId(schema::PrimitiveTypes::kStruct);
            normalize(child);
        }
    }

    syncPadding(container);
}

void FieldTree::resyncPadding()
{
    if (!m_root)
        return;

    normalize(m_root.get());
    emitChangedRecursively(m_root.get());
}

void FieldTree::emitChangedRecursively(const schema::Field *field)
{
    emit fieldChanged(field);

    for (int i = 0; i < field->childCount(); ++i)
        emitChangedRecursively(field->child(i));
}

} // namespace bina::document

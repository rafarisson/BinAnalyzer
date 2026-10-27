#include <vector>

#include "bina/document/PaddingSynchronizer.h"

// bina::schema
#include "bina/schema/Field.h"
#include "bina/schema/LayoutCalculator.h"

namespace bina::document {

PaddingSynchronizer::PaddingSynchronizer(const schema::TypeRegistry *registry, int maxAlign)
    : m_registry{registry}
    , m_maxAlign{maxAlign}
{
}

QList<PaddingSynchronizer::Step> PaddingSynchronizer::plan(const schema::Field *container) const
{
    QList<Step> steps;

    if (!container || !m_registry)
        return steps;

    // Simula as alterações sobre uma cópia da lista de filhos: o tamanho de cada auto_pad, ou -1
    // para um campo.
    std::vector<int> rows;
    int fields = 0;

    for (int i = 0; i < container->childCount(); ++i) {
        const schema::Field *child = container->child(i);
        rows.push_back(child->isAutoPad() ? child->count() : -1);
        fields += child->isAutoPad() ? 0 : 1;
    }

    // Sem campos, o container não tem o que alinhar.
    if (fields == 0) {
        for (int i = int(rows.size()) - 1; i >= 0; --i)
            steps.append({Step::Remove, i});

        return steps;
    }

    const QList<int> wanted = schema::LayoutCalculator(m_registry, m_maxAlign).paddings(container);

    auto isPad = [&rows](size_t row) { return rows[row] >= 0; };

    size_t pos = 0;
    for (int slot = 0; slot <= fields; ++slot) {
        const int size = wanted.value(slot, 0);

        // Pads seguidos nesta posição: mantém o primeiro (se há lacuna) e remove os demais.
        bool kept = false;
        while (pos < rows.size() && isPad(pos)) {
            if (!kept && size > 0) {
                if (rows[pos] != size) {
                    steps.append({Step::Resize, int(pos), size});
                    rows[pos] = size;
                }
                kept = true;
                ++pos;
            } else {
                steps.append({Step::Remove, int(pos)});
                rows.erase(rows.begin() + qsizetype(pos));
            }
        }

        if (!kept && size > 0) {
            steps.append({Step::Insert, int(pos), size});
            rows.insert(rows.begin() + qsizetype(pos), size);
            ++pos;
        }

        if (slot < fields)
            ++pos;      // o campo em si
    }

    return steps;
}

} // namespace bina::document

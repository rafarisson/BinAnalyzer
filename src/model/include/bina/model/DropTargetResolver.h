#pragma once

#include <optional>

#include "bina/document/FieldTree.h"

namespace bina::schema { class Field; }

namespace bina::model {

// Onde uma soltura cai na estrutura: o alvo e a zona, como o FieldTree espera.
struct DropTarget {
    schema::Field *target{nullptr};
    document::FieldTree::DropZone zone{document::FieldTree::Onto};
};

// Traduz (pai, linha) do protocolo de drop do Qt para (alvo, zona) do FieldTree: linha -1 é
// "sobre o pai"; linha >= 0 é entre os filhos, ignorando os auto_pad (são recalculados). Um pai
// sem campos recebe a soltura nele. Vazio só se 'parent' é nulo.
std::optional<DropTarget> resolveDropTarget(schema::Field *parent, int row);

} // namespace bina::model

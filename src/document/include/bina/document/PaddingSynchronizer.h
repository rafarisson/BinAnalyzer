#pragma once

#include <QList>

namespace bina::schema { class Field; class TypeRegistry; }

namespace bina::document {

// Regra dos auto_pad, separada da edição da árvore: um auto_pad antes de cada campo que precisa de
// alinhamento e um ao final, cada um com o tamanho da lacuna (calculada pelo LayoutCalculator).
//
// plan() não altera nada: devolve os passos que levam os filhos de um container ao estado correto,
// na ordem em que devem ser aplicados (cada 'row' vale depois dos passos anteriores). Quem aplica
// (FieldTree) avisa as views a cada passo. Os auto_pad existentes são reaproveitados na mesma
// posição, para não mexer à toa nas linhas das views.
class PaddingSynchronizer
{
public:

    struct Step {
        enum Kind {
            Remove,     // remove o auto_pad da linha 'row'
            Insert,     // insere um auto_pad de 'size' bytes na linha 'row'
            Resize      // muda o tamanho do auto_pad da linha 'row' para 'size'
        };

        Kind kind;
        int row;
        int size{0};

        bool operator==(const Step &o) const { return kind == o.kind && row == o.row && size == o.size; }
    };

    PaddingSynchronizer(const schema::TypeRegistry *registry, int maxAlign);

    // Sem campos (só auto_pad, ou nada), o plano remove todos os filhos.
    QList<Step> plan(const schema::Field *container) const;

private:

    const schema::TypeRegistry *m_registry;
    int m_maxAlign;
};

} // namespace bina::document

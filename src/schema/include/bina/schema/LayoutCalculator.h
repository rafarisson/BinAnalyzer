#pragma once

#include <QHash>
#include <QList>
#include <QString>

#include "bina/schema/OperationResult.h"

namespace bina::schema {

class Field;
class TypeRegistry;

// Calcula tamanhos, alinhamentos e padding dos campos. O alinhamento máximo é parâmetro do cálculo
// (pertence ao documento), então o mesmo registro de tipos serve a qualquer valor.
//
// O layout de cada tipo struct customizado é guardado na primeira vez que é calculado. Por isso um
// calculador vale para um estado do registro: crie outro depois de alterar os tipos.
class LayoutCalculator
{
public:

    struct Layout {
        int size{0};
        int align{1};
    };

    struct PadSlot {
        int beforeIndex{0};
        int size{0};

        bool operator==(const PadSlot &o) const {
            return beforeIndex == o.beforeIndex && size == o.size;
        }
    };

    // 'maxAlign' que não é potência de 2 (0, negativo...) é tratado como 1: nenhum campo é alinhado.
    LayoutCalculator(const TypeRegistry *registry, int maxAlign);

    int maxAlign() const { return m_maxAlign; }

    // Layout de uma instância do campo. Campos com filhos (tipo struct) usam o layout dos filhos.
    Layout layoutOf(const Field *field) const;

    // Tamanho do campo (layout * count); 0 se inválido (tipo desconhecido ou struct vazia).
    int sizeOf(const Field *field) const;

    // Layout dos filhos de field; size == 0 indica layout inválido.
    Layout structLayout(const Field *field, QList<PadSlot> *plan = nullptr) const;
    QList<PadSlot> padPlan(const Field *field) const;

    // Padding de cada lacuna de alinhamento, por posição entre os campos (sem contar os auto_pad):
    // o item i é o padding antes do campo i e o último é o padding final. Tamanho = campos + 1;
    // vazio se o layout for inválido.
    QList<int> paddings(const Field *field) const;

private:

    const TypeRegistry *m_registry;
    int m_maxAlign;
    mutable QHash<QString, Layout> m_typeLayouts;
};

} // namespace bina::schema

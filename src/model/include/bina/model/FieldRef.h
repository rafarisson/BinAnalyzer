#pragma once

#include <QList>
#include <QString>

namespace bina::schema { class Field; }

namespace bina::model {

// Referência a um campo da árvore por posição (caminho de linhas a partir da raiz). Como um caminho
// pode passar a apontar para outro campo se a árvore mudar, a referência carrega o que é preciso
// para detectar isso: o model de origem, a revisão da árvore e o id do campo (schema::Field::id()).
struct FieldRef
{
    quint64 modelToken = 0;     // identifica o model que criou a referência (0 = inválida)
    quint32 revision = 0;       // revisão da árvore quando a referência foi criada
    quint64 fieldId = 0;        // schema::Field::id() do campo, conferido ao resolver
    QList<int> path;            // a raiz tem caminho vazio e não é referenciável

    bool isValid() const { return modelToken != 0 && !path.isEmpty(); }

    static FieldRef fromNode(const schema::Field *field, const schema::Field *root, quint64 modelToken, quint32 revision);

    // Só resolve se o token confere, o caminho existe e o campo encontrado é o mesmo (mesmo id).
    // A revisão é conferida por quem conhece a árvore.
    static schema::Field *toNode(const FieldRef &ref, schema::Field *root, quint64 modelToken);
};

} // namespace bina::model

#pragma once

#include <QLatin1StringView>
#include <QVariant>

namespace bina::schema { class Field; }
namespace bina::document { class FieldTree; }

namespace bina::model {

// Descrição de uma coluna do SchemaTreeModel: as regras de leitura, escrita, edição e exibição
// ficam juntas, numa linha da tabela, em vez de espalhadas por data/setData/flags/headerData.
struct SchemaColumn {
    int role;                       // role equivalente (SchemaTreeModel::Role)
    const char *header;             // texto do cabeçalho, a traduzir no contexto do model
    QLatin1StringView editor;       // EditorKinds; vazio: não editável
    bool alignRight;

    QVariant (*read)(const document::FieldTree &tree, const schema::Field &field);

    // nullptr: só leitura.
    bool (*write)(document::FieldTree &tree, schema::Field *field, const QVariant &value);

    // Regra extra de edição conforme o campo; nullptr: sempre editável (se 'editor' não é vazio).
    bool (*editable)(const schema::Field &field);

    // Opções do combo; nullptr: sem opções.
    QVariantList (*choices)(const document::FieldTree &tree, const schema::Field &field);
};

namespace SchemaColumns {

// A coluna 'column' (SchemaTreeModel::Column), ou nullptr fora do intervalo.
const SchemaColumn *at(int column);

// A coluna cujo role é 'role', ou nullptr.
const SchemaColumn *forRole(int role);

}

} // namespace bina::model

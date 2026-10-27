#include <QtGlobal>

#include "bina/model/SchemaColumns.h"
#include "bina/model/EditorRoles.h"
#include "bina/model/SchemaTreeModel.h"

// bina::schema
#include "bina/schema/Field.h"
#include "bina/document/FieldTree.h"

namespace bina::model {

namespace {

using document::FieldTree;
using schema::Field;

// Mesmo contexto de tradução de tr() em SchemaTreeModel.
#define BINA_COLUMN_HEADER(text) QT_TRANSLATE_NOOP("bina::model::SchemaTreeModel", text)

const SchemaColumn kColumns[] = {
    // NameColumn
    {
        SchemaTreeModel::NameRole, BINA_COLUMN_HEADER("Nome"), EditorKinds::kText, false,
        [](const FieldTree &, const Field &f) { return QVariant(f.name()); },
        [](FieldTree &t, Field *f, const QVariant &v) { return bool(t.setName(f, v.toString())); },
        nullptr,
        nullptr
    },
    // TypeIdColumn: o tipo de um campo com filhos é sempre struct.
    {
        SchemaTreeModel::TypeIdRole, BINA_COLUMN_HEADER("Tipo"), EditorKinds::kCombo, false,
        [](const FieldTree &, const Field &f) { return QVariant(f.typeId()); },
        [](FieldTree &t, Field *f, const QVariant &v) { return bool(t.setTypeId(f, v.toString())); },
        [](const Field &f) { return f.childCount() == 0; },
        [](const FieldTree &t, const Field &) { return makeChoices(t.typeIds()); }
    },
    // CountColumn
    {
        SchemaTreeModel::CountRole, BINA_COLUMN_HEADER("Qtde"), EditorKinds::kSpin, true,
        [](const FieldTree &, const Field &f) { return QVariant(f.count()); },
        [](FieldTree &t, Field *f, const QVariant &v) { return bool(t.setCount(f, v.toInt())); },
        nullptr,
        nullptr
    },
    // SizeInBytesColumn: calculado.
    {
        SchemaTreeModel::SizeInBytesRole, BINA_COLUMN_HEADER("Total"), {}, true,
        [](const FieldTree &t, const Field &f) { return QVariant(t.sizeInBytes(&f)); },
        nullptr,
        nullptr,
        nullptr
    },
    // FormatIdColumn
    {
        SchemaTreeModel::FormatIdRole, BINA_COLUMN_HEADER("Exibição"), EditorKinds::kCombo, false,
        [](const FieldTree &, const Field &f) { return QVariant(f.formatId()); },
        [](FieldTree &t, Field *f, const QVariant &v) { return bool(t.setFormatId(f, v.toString())); },
        nullptr,
        [](const FieldTree &t, const Field &f) { return makeChoices(t.compatibleFormatIds(&f), true); }
    },
};

#undef BINA_COLUMN_HEADER

static_assert(std::size(kColumns) == SchemaTreeModel::ColumnCount);

} // namespace

const SchemaColumn *SchemaColumns::at(int column)
{
    if (column < 0 || column >= int(std::size(kColumns)))
        return nullptr;

    return &kColumns[column];
}

const SchemaColumn *SchemaColumns::forRole(int role)
{
    for (const SchemaColumn &column : kColumns) {
        if (column.role == role)
            return &column;
    }

    return nullptr;
}

} // namespace bina::model

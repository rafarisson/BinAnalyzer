#pragma once

#include <memory>

#include <QFile>
#include <QStringList>
#include <QtTest>

#include "bina/document/Document.h"
#include "bina/document/DocumentFactory.h"
#include "bina/document/FieldTree.h"
#include "bina/io/JsonDocumentImporter.h"
#include "bina/schema/DataType.h"
#include "bina/schema/Field.h"
#include "bina/schema/LayoutCalculator.h"

// Apoio comum aos testes.
namespace TestSupport {

inline QString sample(const QString &name)
{
    return QStringLiteral(BINA_SAMPLES_DIR "/") + name;
}

inline QByteArray readFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

// Cria um documento a partir de JSON; devolve nullptr (e o motivo em 'error') se for inválido.
inline std::unique_ptr<bina::document::Document> documentFromJson(const QByteArray &json, QString *error = nullptr)
{
    bina::io::JsonDocumentImporter importer;
    bina::document::DocumentDto dto;

    if (const bina::schema::OperationResult read = importer.parse(json, dto); !read) {
        if (error)
            *error = read.error();
        return nullptr;
    }

    auto doc = std::make_unique<bina::document::Document>();
    bina::document::DocumentFactory factory;

    if (const bina::schema::OperationResult created = factory.create(dto, *doc); !created) {
        if (error)
            *error = created.error();
        return nullptr;
    }

    return doc;
}

inline std::unique_ptr<bina::document::Document> sampleDocument(const QString &name = QStringLiteral("elements.json"))
{
    return documentFromJson(readFile(sample(name)));
}

// Nomes dos filhos; os auto_pad aparecem como "auto_pad" só se pedidos.
inline QStringList childNames(const bina::schema::Field *field, bool withPads = false)
{
    QStringList names;

    for (int i = 0; i < field->childCount(); ++i) {
        const bina::schema::Field *child = field->child(i);

        if (withPads || !child->isAutoPad())
            names << child->name();
    }

    return names;
}

// Tamanhos dos auto_pad entre os filhos, na ordem.
inline QList<int> padSizes(const bina::schema::Field *field)
{
    QList<int> sizes;

    for (int i = 0; i < field->childCount(); ++i)
        if (field->child(i)->isAutoPad())
            sizes << field->child(i)->count();

    return sizes;
}

// Campo por caminho de nomes ("header.bounds.min"); ignora os auto_pad.
inline bina::schema::Field *findField(bina::schema::Field *parent, const QString &path)
{
    bina::schema::Field *node = parent;

    for (const QString &name : path.split(QLatin1Char('.'))) {
        bina::schema::Field *next = nullptr;

        for (int i = 0; node && i < node->childCount() && !next; ++i) {
            bina::schema::Field *child = node->child(i);

            if (!child->isAutoPad() && child->name() == name)
                next = child;
        }

        node = next;
    }

    return node;
}

} // namespace TestSupport

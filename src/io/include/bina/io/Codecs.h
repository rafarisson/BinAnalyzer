#pragma once

#include <functional>
#include <memory>

#include <QList>
#include <QString>
#include <QStringList>

#include "bina/document/DocumentDto.h"
#include "bina/io/DocumentImporter.h"
#include "bina/io/Exporter.h"
#include "bina/io/ResultDto.h"

namespace bina::io {

// Formatos de arquivo conhecidos pela aplicação, escolhidos pela extensão. Um formato novo (YAML,
// header C...) é registrado aqui, sem mudar quem abre ou salva.

// Um formato de arquivo da estrutura. 'importer' ou 'exporter' vazios: o formato só é lido ou só é
// escrito.
struct DocumentCodec
{
    QString suffix;         // sem ponto, em minúsculas ("json")
    QString name;           // para filtros de diálogo ("Estrutura JSON")
    std::function<std::unique_ptr<DocumentImporter>()> importer;
    std::function<std::unique_ptr<Exporter<document::DocumentDto>>()> exporter;
};

class DocumentCodecRegistry
{
public:

    // Com os formatos embutidos (JSON).
    static DocumentCodecRegistry withBuiltins();

    void add(const DocumentCodec &codec);

    // Pela extensão do caminho (sem diferenciar maiúsculas); nullptr se nenhum formato serve.
    const DocumentCodec *forPath(const QString &path) const;

    // A extensão usada quando o caminho não tem uma (a do primeiro formato registrado).
    QString defaultSuffix() const;

    QList<DocumentCodec> codecs() const { return m_codecs; }

private:

    QList<DocumentCodec> m_codecs;
};

// Um formato de exportação do resultado. Os resultados têm duas formas, e cada formato usa uma:
// a árvore (como a view de árvore) ou a tabela (um registro por linha).
struct ResultCodec
{
    enum Shape {
        Tree,
        Table
    };

    QString suffix;
    QString name;
    Shape shape;
    std::function<std::unique_ptr<Exporter<ResultTreeDto>>()> treeExporter;     // shape == Tree
    std::function<std::unique_ptr<Exporter<ResultTableDto>>()> tableExporter;   // shape == Table
};

class ResultCodecRegistry
{
public:

    // Com os formatos embutidos: JSON (árvore) e CSV (tabela).
    static ResultCodecRegistry withBuiltins();

    void add(const ResultCodec &codec);
    const ResultCodec *forPath(const QString &path) const;
    QString defaultSuffix() const;
    QStringList suffixes() const;

private:

    QList<ResultCodec> m_codecs;
};

} // namespace bina::io

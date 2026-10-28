#pragma once

#include <deque>

#include <QList>
#include <QString>

#include "bina/format/Formatter.h"
#include "bina/format/Value.h"

namespace bina::schema { class Field; }

namespace bina::parse {

// Resultado da leitura de um campo do binário. A árvore espelha a estrutura do schema:
//  - campo escalar: sem filhos;
//  - campo struct com count == 1: um filho por campo (sem os auto_pad);
//  - campo struct com count > 1: um filho por instância ("nome[i]"), cada uma com seus campos.
//
// O valor é uma cópia dos bytes decodificados, então o resultado não depende do buffer de entrada.
// 'field' aponta para o schema usado e deixa de ser válido se o schema for alterado:
// nesse caso o arquivo deve ser analisado novamente.
struct ParsedNode
{
    const schema::Field *field{nullptr};
    QString name;
    qint64 instance{-1};        // índice do registro ou da instância em um array de structs, ou -1
    qint64 offset{0};           // posição no arquivo
    qint64 size{0};             // bytes ocupados (inclui count e padding interno)
    bool truncated{false};      // o arquivo termina antes do fim do campo
    format::Value value;        // inválido se o campo está truncado
    format::Target target;      // tipo e count no momento da análise (count = 1 nas instâncias)
    QList<ParsedNode> children;
};

// Um arquivo é uma sequência de registros, cada um com a estrutura do schema (Field raiz).
// Um arquivo menor que um registro, ou com um registro final incompleto, gera um último
// registro truncado; um arquivo vazio gera um único registro truncado.
//
// Os registros são lidos sob demanda (BinaryParser::parseMore()): 'records' tem os já lidos, em
// ordem, e recordCount() diz quantos o arquivo tem. É um std::deque para que acrescentar registros
// não mova os já lidos: quem guarda ponteiros para os nós (os models) continua válido.
struct ParseResult
{
    const schema::Field *structure{nullptr};    // a raiz do schema usada na análise
    quint64 generation{0};      // análise que produziu o resultado (ver AnalysisSession)
    std::deque<ParsedNode> records;
    qint64 recordSize{0};       // tamanho de um registro segundo o schema
    qint64 dataSize{0};         // tamanho do arquivo analisado
    qint64 dataOffset{0};       // onde começam os registros: o tamanho do cabeçalho, se houver

    // Bytes do arquivo depois do cabeçalho: os que formam registros.
    qint64 payloadSize() const { return qMax<qint64>(0, dataSize - dataOffset); }

    // Registros do arquivo, lidos ou não (inclui o último, se incompleto).
    qint64 recordCount() const
    {
        return recordSize > 0 ? qMax<qint64>(1, (payloadSize() + recordSize - 1) / recordSize) : 0;
    }

    qint64 parsedCount() const { return qint64(records.size()); }
    bool isComplete() const { return parsedCount() >= recordCount(); }

    // Registros que cabem inteiros no arquivo.
    qint64 completeRecords() const { return recordSize > 0 ? payloadSize() / recordSize : 0; }

    // Bytes do último registro, incompleto (0 se o arquivo termina em um limite de registro).
    qint64 incompleteBytes() const { return recordSize > 0 ? payloadSize() % recordSize : 0; }
};

} // namespace bina::parse

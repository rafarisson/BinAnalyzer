#pragma once

#include <QByteArrayView>
#include <QString>
#include <QSysInfo>

#include "bina/parse/ParsedNode.h"
#include "bina/schema/LayoutCalculator.h"
#include "bina/schema/OperationResult.h"

namespace bina::schema { class Field; class TypeRegistry; }

namespace bina::parse {

// Lê um binário segundo um schema (Field raiz + TypeRegistry) e produz os valores decodificados.
// Não formata nada: a formatação é aplicada depois sobre os Values, sem reanalisar o arquivo.
//
// Um parser vale para um estado do registro de tipos (o layout dos tipos fica em cache): depois de
// alterar os tipos, crie outro.
class BinaryParser
{
public:

    // 'maxAlign' que não é potência de 2 é tratado como 1 (ver LayoutCalculator).
    BinaryParser(const schema::TypeRegistry *types, QSysInfo::Endian endian, int maxAlign);

    // O arquivo é lido como uma sequência de registros da estrutura 'root' (ver ParseResult). Lê
    // até 'maxRecords' registros (todos, se negativo); os demais vêm com parseMore().
    // Falha se o schema for inválido (tipo desconhecido, struct vazia). Dados faltando não são
    // erro: os campos além do fim ficam marcados como truncados.
    schema::OperationResult parse(const schema::Field *root, QByteArrayView data, ParseResult &out,
                                  qint64 maxRecords = -1);

    // Acrescenta a 'result' os próximos 'count' registros (todos os que faltam, se negativo).
    // 'data' e o schema devem ser os mesmos usados em parse().
    schema::OperationResult parseMore(ParseResult &result, QByteArrayView data, qint64 count = -1);

private:

    bool parseFields(const schema::Field *definition, qint64 base, QByteArrayView data, QList<ParsedNode> &out);
    bool parseField(const schema::Field *field, qint64 offset, QByteArrayView data, ParsedNode &out);
    bool parseStructInstance(const schema::Field *definition, qint64 offset, qint64 size, QByteArrayView data, ParsedNode &out);

    bool fail(const QString &message);

    const schema::TypeRegistry *m_types;
    QSysInfo::Endian m_endian;
    schema::LayoutCalculator m_calculator;
    QString m_error;
};

} // namespace bina::parse

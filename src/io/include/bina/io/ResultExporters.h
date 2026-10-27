#pragma once

#include "bina/io/Exporter.h"
#include "bina/io/ResultDto.h"

namespace bina::io {

// Árvore de resultado em JSON: uma lista de nós com name, type, offset, bytes, value e, quando o
// campo tem filhos, children.
class JsonResultExporter : public Exporter<ResultTreeDto>
{
public:

    bool write(QIODevice *device, const ResultTreeDto &data) override;
};

// Tabela de resultado em CSV: o cabeçalho com o nome dos campos e uma linha por registro. As células
// saem como estão; só as que têm o separador, aspas ou quebra de linha vão entre aspas. Começa com a
// marca UTF-8 para o Excel acertar os acentos.
class CsvResultExporter : public Exporter<ResultTableDto>
{
public:

    explicit CsvResultExporter(QChar separator = QLatin1Char(','));

    bool write(QIODevice *device, const ResultTableDto &data) override;

private:

    QChar m_separator;
};

} // namespace bina::io

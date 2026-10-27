#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace bina::io {

// O resultado da análise como a tela o mostra: só texto legível, sem a estrutura, os tipos nem os
// formatos do documento. É o que os exportadores de resultado recebem.

// Um campo da árvore de resultado.
struct ResultNodeDto
{
    QString name;
    QString type;
    QString offset;     // como exibido, por exemplo "0x0010"
    qint64 bytes{0};
    QString value;      // já formatado
    QList<ResultNodeDto> children;
};

struct ResultTreeDto
{
    QList<ResultNodeDto> nodes;
};

// A tabela de resultado: um cabeçalho com o nome dos campos e uma linha por registro.
struct ResultTableDto
{
    QStringList headers;
    QList<QStringList> rows;
};

} // namespace bina::io

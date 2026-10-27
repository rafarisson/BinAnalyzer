#pragma once

#include <QString>

#include "bina/format/Value.h"

namespace bina::format {

// Experimento com um padrão printf: o usuário digita o padrão e uma amostra de valores e vê o
// texto que sairia. Nada aqui altera o documento.
//
// A amostra é uma lista separada por vírgula, ponto e vírgula ou quebra de linha:
//   "major=1, minor=2, patch=3"   campos nomeados (um caminho "a.b" cria um struct dentro de outro)
//   "1, 2.5, texto"               valores posicionais, na ordem das conversões do padrão
// Um valor é inteiro (decimal ou 0x...), real ou, nos demais casos, texto.

struct PatternPreviewResult
{
    bool patternValid{false};   // o padrão compila (a amostra pode estar errada mesmo assim)
    bool ok{false};
    QString text;       // o resultado, se ok
    QString error;      // o motivo, se não ok
};

// Monta o Value da amostra; 'error' recebe o motivo quando a amostra é inválida (e o Value fica vazio).
Value parseSample(const QString &sample, QString *error = nullptr);

// Uma amostra que serve de ponto de partida para o padrão (vazia se o padrão é inválido).
QString suggestSample(const QString &spec);

PatternPreviewResult previewPattern(const QString &spec, const QString &sample);

} // namespace bina::format

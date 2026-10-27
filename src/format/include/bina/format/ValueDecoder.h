#pragma once

#include <QByteArrayView>
#include <QString>
#include <QSysInfo>

#include "bina/schema/Types.h"
#include "bina/format/Value.h"

namespace bina::format {

// Converte os bytes de um campo (fieldSize * count) em Value.
//  - count == 1: escalar (inteiro, float, ou Bytes se não couber em 8 bytes);
//  - count > 1: Text para char, Bytes para opaco e Array para os demais.
Value decodeValue(Representation representation, int fieldSize, int count, QByteArrayView bytes, QSysInfo::Endian endian);

// Texto de bytes lidos do arquivo: UTF-8 quando os bytes formam UTF-8 válido, senão Latin-1 (cada
// byte vira um caractere). Assim um texto binário comum não vira U+FFFD. Usado por char[N] e pela
// conversão "%s" de buffers.
QString decodeText(QByteArrayView bytes);

}

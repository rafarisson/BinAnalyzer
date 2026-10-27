#pragma once

#include <QMimeData>

namespace bina::model { struct FieldRef; }

namespace bina::model {

// Serialização de uma referência a campo para arrastar e soltar.
//
// Formato (QDataStream, Qt 6.0): magic 'BINA', versão, token do model, revisão da árvore,
// identificação do campo e caminho. A decodificação não confia nos dados: confere magic e versão,
// limita tamanhos e profundidade e recusa dados sobrando ou truncados. Quem usa o resultado ainda
// precisa conferir o token e a revisão (ver FieldRef::toNode).
class FieldMimeCodec
{
public:

    static constexpr int MaxPathDepth = 64;

    static QMimeData *encode(const FieldRef &ref);
    static bool decode(const QMimeData *mime, FieldRef &ref);
    static bool canDecode(const QMimeData *mime);
    static QStringList mimeTypes();
};

} // namespace bina::model

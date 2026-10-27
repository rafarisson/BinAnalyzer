#pragma once

#include <QByteArray>
#include <QString>

#include "bina/document/DocumentDto.h"
#include "bina/io/Exporter.h"

namespace bina::io {

// A estrutura (tipos, formatos e campos) do documento em JSON.
class JsonDocumentExporter : public Exporter<document::DocumentDto>
{
public:

    bool write(QIODevice *device, const document::DocumentDto &data) override;
};

} // namespace bina::io

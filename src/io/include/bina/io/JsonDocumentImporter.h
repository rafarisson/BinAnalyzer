#pragma once

#include "bina/document/DocumentDto.h"
#include "bina/io/DocumentImporter.h"

class QJsonObject;

namespace bina::io {

class JsonDocumentImporter : public DocumentImporter
{
public:

protected:

    bool readDevice(QIODevice *device, document::DocumentDto &out) override;

private:

    static constexpr int MaxDepth = 64;

    bool readDocument(const QJsonObject &obj, document::DocumentDto &out);
    bool readFormatters(const QJsonObject &obj, document::DocumentDto &out);
    bool readTypes(const QJsonObject &obj, document::DocumentDto &out);
    bool readFields(const QJsonObject &obj, document::DocumentFieldDto &parent, int depth);
    bool readField(const QJsonObject &obj, document::DocumentFieldDto &out, int depth);
};

} // namespace bina::io

#include <QBuffer>
#include <QFile>

#include "bina/io/DocumentImporter.h"

namespace bina::io {

schema::OperationResult DocumentImporter::read(QIODevice *device, document::DocumentDto &out)
{
    clearStatus();
    out = document::DocumentDto();

    if (!readDevice(device, out)) {
        out = document::DocumentDto();
        return schema::OperationResult::failure(m_error);
    }

    return schema::OperationResult::success();
}

schema::OperationResult DocumentImporter::parse(const QByteArray &content, document::DocumentDto &out)
{
    QBuffer buffer;
    buffer.setData(content);

    if (!buffer.open(QIODevice::ReadOnly)) {
        clearStatus();
        out = document::DocumentDto();
        return schema::OperationResult::failure(buffer.errorString());
    }

    return read(&buffer, out);
}

schema::OperationResult DocumentImporter::readFile(const QString &path, document::DocumentDto &out)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        clearStatus();
        out = document::DocumentDto();
        return schema::OperationResult::failure(file.errorString());
    }

    return read(&file, out);
}

void DocumentImporter::clearStatus()
{
    m_error.clear();
    m_warnings.clear();
}

} // namespace bina::io

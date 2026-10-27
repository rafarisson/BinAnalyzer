#pragma once

#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
#include <QSaveFile>
#include <QString>

#include "bina/schema/OperationResult.h"

namespace bina::io {

// Base dos exportadores, o par do DocumentImporter: cada formato implementa write() para um
// dispositivo e herda daqui a serialização em memória e a gravação segura em arquivo. 'Dto' é o
// conteúdo exportado; um formato novo é uma classe nova que herda de Exporter<Dto>.
template <typename Dto>
class Exporter
{
public:

    virtual ~Exporter() = default;

    // Escreve 'data' no dispositivo (já aberto para escrita). Em falha chama fail() e devolve false.
    virtual bool write(QIODevice *device, const Dto &data) = 0;

    // O conteúdo exportado em 'out'; em falha 'out' fica vazio e o resultado diz o motivo.
    schema::OperationResult serialize(const Dto &data, QByteArray &out)
    {
        using schema::OperationResult;

        m_error.clear();
        out.clear();

        QBuffer buffer;

        if (!buffer.open(QIODevice::WriteOnly))
            return OperationResult::failure(buffer.errorString());

        if (!write(&buffer, data))
            return OperationResult::failure(m_error);

        out = buffer.data();

        return OperationResult::success();
    }

    // Só substitui o arquivo se a escrita inteira der certo.
    schema::OperationResult writeFile(const QString &path, const Dto &data)
    {
        using schema::OperationResult;

        m_error.clear();

        QSaveFile file(path);

        if (!file.open(QIODevice::WriteOnly))
            return OperationResult::failure(file.errorString());

        if (!write(&file, data)) {
            file.cancelWriting();
            return OperationResult::failure(m_error);
        }

        if (!file.commit())
            return OperationResult::failure(file.errorString());

        return OperationResult::success();
    }

protected:

    void fail(const QString &message) { m_error = message; }

    // Escreve os bytes e registra a falha, se houver.
    bool writeAll(QIODevice *device, const QByteArray &bytes)
    {
        if (device->write(bytes) != bytes.size()) {
            fail(device->errorString());
            return false;
        }

        return true;
    }

private:

    QString m_error;
};

} // namespace bina::io

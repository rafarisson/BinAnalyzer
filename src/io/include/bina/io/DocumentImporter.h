#pragma once

#include <QByteArray>
#include <QIODevice>
#include <QStringList>

#include "bina/document/DocumentDto.h"
#include "bina/schema/OperationResult.h"

namespace bina::io {

// Base dos importadores de estrutura: cada formato implementa readDevice() e herda daqui a leitura
// de memória e de arquivo. Em qualquer falha 'out' volta a um DTO vazio.
class DocumentImporter
{
public:

    virtual ~DocumentImporter() = default;

    schema::OperationResult read(QIODevice *device, document::DocumentDto &out);
    schema::OperationResult parse(const QByteArray &content, document::DocumentDto &out);
    schema::OperationResult readFile(const QString &path, document::DocumentDto &out);

    // Avisos da última leitura (chaves desconhecidas e afins); não impedem a leitura.
    QStringList warnings() const { return m_warnings; }

protected:

    // Lê o dispositivo (já aberto). Em falha chama fail() e devolve false.
    virtual bool readDevice(QIODevice *device, document::DocumentDto &out) = 0;

    void fail(const QString &message) { m_error = message; }
    void addWarning(const QString &message) { m_warnings << message; }

private:

    void clearStatus();

    QString m_error;
    QStringList m_warnings;
};

} // namespace bina::io

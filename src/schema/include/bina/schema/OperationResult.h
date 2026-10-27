#pragma once

#include <QString>

namespace bina::schema {

// Resultado de uma operação que pode ser recusada: sucesso, ou falha com a mensagem do motivo.
// Converte para bool, então 'if (!registry.add(...))' continua valendo.
class OperationResult
{
public:

    static OperationResult success() { return OperationResult(true, QString()); }
    static OperationResult failure(const QString &message) { return OperationResult(false, message); }

    bool ok() const { return m_ok; }
    explicit operator bool() const { return m_ok; }

    // Vazio quando a operação foi aceita.
    QString error() const { return m_error; }

private:

    OperationResult(bool ok, const QString &error) : m_ok{ok}, m_error{error} {}

    bool m_ok;
    QString m_error;
};

} // namespace bina::schema

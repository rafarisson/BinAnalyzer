#pragma once

#include <chrono>

#include <QElapsedTimer>

namespace bina::schema { class Field; }

namespace bina::model {

// Depois de um drop com MoveAction a view remove a origem com removeRows(). O campo já foi
// movido, então esse pedido deve ser reconhecido e ignorado: arm() registra a origem e take()
// a reconhece uma única vez, dentro do prazo. Qualquer take() encerra a espera.
class PendingMoveGuard
{
public:

    static constexpr std::chrono::milliseconds kDefaultTimeout{3000};

    explicit PendingMoveGuard(std::chrono::milliseconds timeout = kDefaultTimeout)
        : m_timeout{timeout}
    {
    }

    void arm(const schema::Field *source);
    bool take(const schema::Field *field);
    void reset();

private:

    const schema::Field *m_source{nullptr};
    QElapsedTimer m_age;
    std::chrono::milliseconds m_timeout;
};

} // namespace bina::model

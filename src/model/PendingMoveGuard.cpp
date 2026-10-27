#include "bina/model/PendingMoveGuard.h"

namespace bina::model {

void PendingMoveGuard::arm(const schema::Field *source)
{
    m_source = source;
    m_age.start();
}

bool PendingMoveGuard::take(const schema::Field *field)
{
    const bool match = field && field == m_source
                       && m_age.isValid() && m_age.durationElapsed() < m_timeout;

    reset();

    return match;
}

void PendingMoveGuard::reset()
{
    m_source = nullptr;
    m_age.invalidate();
}

} // namespace bina::model

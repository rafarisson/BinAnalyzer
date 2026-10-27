#include <QRegularExpression>

#include "bina/schema/Identifier.h"

namespace bina::schema {

QString normalizeIdentifier(const QString &id)
{
    static const QRegularExpression re(QStringLiteral("[^\\p{L}\\p{N}_]"));
    QString normalized = id.trimmed();
    normalized.replace(re, QStringLiteral("_"));
    return normalized;
}

} // namespace bina::schema

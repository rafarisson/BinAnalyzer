#include <QCoreApplication>
#include <QRegularExpression>

#include "bina/format/FormatRegistry.h"
#include "bina/format/BuiltinFormatters.h"
#include "bina/format/PrintfFormat.h"

namespace bina::format {

FormatRegistry::FormatRegistry()
    : m_builtin{createBuiltinFormatters()}
{
}

const Formatter *FormatRegistry::find(const QString &id) const
{
    for (const auto &f : m_builtin)
        if (f->id() == id)
            return f.get();

    for (const auto &f : m_named)
        if (f->id() == id)
            return f.get();

    if (!id.startsWith(FormatIds::kPrintfPrefix))
        return nullptr;

    if (const auto it = m_inline.constFind(id); it != m_inline.constEnd())
        return it->get();

    const PrintfFormat compiled = PrintfFormat::compile(id.mid(FormatIds::kPrintfPrefix.size()));
    if (!compiled.isValid())
        return nullptr;

    // Um cache cheio recomeça vazio: os padrões ainda em uso são recompilados na próxima consulta.
    if (m_inline.size() >= kMaxInlineCache)
        m_inline.clear();

    auto formatter = std::make_shared<PrintfFormatter>(id, compiled);
    m_inline.insert(id, formatter);

    return formatter.get();
}

schema::OperationResult FormatRegistry::check(const QString &id) const
{
    using schema::OperationResult;

    if (find(id))
        return OperationResult::success();

    if (id.startsWith(FormatIds::kPrintfPrefix)) {
        const PrintfFormat compiled = PrintfFormat::compile(id.mid(FormatIds::kPrintfPrefix.size()));
        return OperationResult::failure(QCoreApplication::translate("bina", "Formato '%1' inválido: %2").arg(id, compiled.errorString()));
    }

    return OperationResult::failure(QCoreApplication::translate("bina", "Formato desconhecido: '%1'").arg(id));
}

schema::OperationResult FormatRegistry::addPattern(const QString &id, const QString &spec)
{
    using schema::OperationResult;

    static const QRegularExpression idRe(QStringLiteral("^[\\p{L}_][\\p{L}\\p{N}_]*$"));

    if (!idRe.match(id).hasMatch())
        return OperationResult::failure(QCoreApplication::translate("bina", "Id de formato inválido: '%1'").arg(id));

    if (contains(id))
        return OperationResult::failure(QCoreApplication::translate("bina", "O formato '%1' já existe").arg(id));

    const PrintfFormat compiled = PrintfFormat::compile(spec);
    if (!compiled.isValid())
        return OperationResult::failure(QCoreApplication::translate("bina", "Padrão inválido em '%1': %2").arg(id, compiled.errorString()));

    m_named.push_back(std::make_unique<PrintfFormatter>(id, compiled));
    m_patterns.append({id, spec});

    return OperationResult::success();
}

schema::OperationResult FormatRegistry::setPattern(const QString &id, const QString &spec)
{
    using schema::OperationResult;

    qsizetype index = -1;
    for (qsizetype i = 0; i < m_patterns.size(); ++i) {
        if (m_patterns.at(i).id == id)
            index = i;
    }

    if (index < 0)
        return OperationResult::failure(QCoreApplication::translate("bina", "O formato '%1' não é um padrão customizado").arg(id));

    const PrintfFormat compiled = PrintfFormat::compile(spec);
    if (!compiled.isValid())
        return OperationResult::failure(QCoreApplication::translate("bina", "Padrão inválido em '%1': %2").arg(id, compiled.errorString()));

    // m_named e m_patterns têm a mesma ordem.
    m_named[size_t(index)] = std::make_unique<PrintfFormatter>(id, compiled);
    m_patterns[index].spec = spec;

    return OperationResult::success();
}

QList<const Formatter *> FormatRegistry::formatters() const
{
    QList<const Formatter *> out;
    for (const auto &f : m_builtin)
        out << f.get();
    for (const auto &f : m_named)
        out << f.get();
    return out;
}

QList<const Formatter *> FormatRegistry::compatible(const Target &target) const
{
    QList<const Formatter *> out;
    for (const Formatter *f : formatters())
        if (f->accepts(target))
            out << f;
    return out;
}

bool FormatRegistry::isCompatible(const QString &id, const Target &target) const
{
    const Formatter *f = find(id);
    return f && f->accepts(target);
}

QString FormatRegistry::defaultId(const Target &target) const
{
    if (target.isStruct)
        return {};

    switch (target.representation) {
    case Representation::Signed:
    case Representation::Unsigned:
    case Representation::Enum:
        return FormatIds::kDecimal;
    case Representation::Float:
        return FormatIds::kFloat;
    case Representation::Char:
        return FormatIds::kString;
    default:
        return FormatIds::kBytes;
    }
}

QString FormatRegistry::format(const QString &id, const Target &target, const Value &value) const
{
    const Formatter *f = find(id.isEmpty() ? defaultId(target) : id);
    return f ? f->format(value) : QString();
}

}

#include <QCoreApplication>
#include <QHash>
#include <QRegularExpression>
#include <QSet>

#include "bina/format/PatternPreview.h"
#include "bina/format/PrintfFormat.h"

namespace bina::format {

namespace {

struct Entry {
    QString path;       // vazio: valor posicional
    QString text;
};

Value scalarFrom(const QString &raw)
{
    static const QRegularExpression integerRe(QStringLiteral("^[+-]?\\d+$"));
    static const QRegularExpression hexRe(QStringLiteral("^0[xX][0-9a-fA-F]+$"));

    const QString text = raw.trimmed();

    if (integerRe.match(text).hasMatch()) {
        bool ok = false;
        const qint64 v = text.toLongLong(&ok);

        if (ok) {
            return v < 0 ? Value::makeSigned(v, 8, Representation::Signed)
                         : Value::makeUnsigned(quint64(v), 8, Representation::Unsigned);
        }
    }

    if (hexRe.match(text).hasMatch()) {
        bool ok = false;
        const quint64 v = text.toULongLong(&ok, 16);

        if (ok)
            return Value::makeUnsigned(v, 8, Representation::Unsigned);
    }

    bool isReal = false;
    const double real = text.toDouble(&isReal);
    if (isReal && !text.isEmpty())
        return Value::makeFloat(real, 8);

    return Value::makeText(text, text.toLatin1());
}

// Agrupa os campos pelo primeiro trecho do caminho, na ordem em que aparecem; os posicionais ficam
// onde estão. Um nome que tem valor e também filhos fica com o valor.
Value buildStruct(const QList<Entry> &entries)
{
    QStringList names;
    QList<Value> values;
    QHash<QString, int> indexOf;

    for (const Entry &entry : entries) {
        if (entry.path.isEmpty()) {
            names << QString();
            values << scalarFrom(entry.text);
            continue;
        }

        const qsizetype dot = entry.path.indexOf(QLatin1Char('.'));
        const QString head = dot < 0 ? entry.path : entry.path.left(dot);

        if (dot < 0) {
            const Value value = scalarFrom(entry.text);

            if (indexOf.contains(head)) {
                values[indexOf.value(head)] = value;
            } else {
                indexOf.insert(head, int(names.size()));
                names << head;
                values << value;
            }
            continue;
        }

        if (!indexOf.contains(head)) {
            indexOf.insert(head, int(names.size()));
            names << head;
            values << Value();
        }
    }

    // Segundo passo: os grupos (caminhos com ponto) viram structs.
    for (auto it = indexOf.cbegin(); it != indexOf.cend(); ++it) {
        QList<Entry> children;
        bool hasOwnValue = false;

        for (const Entry &entry : entries) {
            if (entry.path == it.key())
                hasOwnValue = true;
            else if (entry.path.startsWith(it.key() + QLatin1Char('.')))
                children.append({entry.path.mid(it.key().size() + 1), entry.text});
        }

        if (!hasOwnValue && !children.isEmpty())
            values[it.value()] = buildStruct(children);
    }

    return Value::makeStruct(names, values);
}

QString defaultSampleFor(QChar conversion)
{
    switch (conversion.toLatin1()) {
    case 'f': case 'F': case 'e': case 'E': case 'g': case 'G':
        return QStringLiteral("1.5");
    case 's':
        return QStringLiteral("texto");
    case 'c':
        return QStringLiteral("65");
    default:
        return QStringLiteral("1");
    }
}

} // namespace

Value parseSample(const QString &sample, QString *error)
{
    static const QRegularExpression separator(QStringLiteral("[,;\\n]"));
    static const QRegularExpression pathRe(QStringLiteral("^[\\p{L}_][\\p{L}\\p{N}_]*(\\.[\\p{L}_][\\p{L}\\p{N}_]*)*$"));

    QList<Entry> entries;

    for (const QString &token : sample.split(separator, Qt::SkipEmptyParts)) {
        const QString trimmed = token.trimmed();
        if (trimmed.isEmpty())
            continue;

        const qsizetype eq = trimmed.indexOf(QLatin1Char('='));
        if (eq < 0) {
            entries.append({QString(), trimmed});
            continue;
        }

        const QString path = trimmed.left(eq).trimmed();
        if (!pathRe.match(path).hasMatch()) {
            if (error)
                *error = QCoreApplication::translate("bina", "Nome de campo inválido: '%1'").arg(path);
            return {};
        }

        entries.append({path, trimmed.mid(eq + 1)});
    }

    if (error)
        error->clear();

    return buildStruct(entries);
}

QString suggestSample(const QString &spec)
{
    const PrintfFormat format = PrintfFormat::compile(spec);
    if (!format.isValid())
        return {};

    QStringList parts;
    QSet<QString> seen;
    int position = 0;

    const QList<PrintfFormat::Reference> references = format.references();
    for (const PrintfFormat::Reference &ref : references) {
        const QString value = defaultSampleFor(ref.conversion);

        if (ref.key.isEmpty()) {
            const bool isInteger = QStringLiteral("diuxXob").contains(ref.conversion);
            parts << (isInteger ? QString::number(++position) : value);
        } else if (!seen.contains(ref.key)) {
            seen.insert(ref.key);
            parts << ref.key + QLatin1Char('=') + value;
        }
    }

    return parts.join(QStringLiteral(", "));
}

PatternPreviewResult previewPattern(const QString &spec, const QString &sample)
{
    PatternPreviewResult result;

    const PrintfFormat format = PrintfFormat::compile(spec);
    if (!format.isValid()) {
        result.error = format.errorString();
        return result;
    }

    result.patternValid = true;

    QString error;
    const Value value = parseSample(sample, &error);
    if (!error.isEmpty()) {
        result.error = error;
        return result;
    }

    result.ok = true;
    result.text = format.format(value);

    return result;
}

} // namespace bina::format

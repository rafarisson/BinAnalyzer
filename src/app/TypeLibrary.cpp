#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include "bina/app/MessageLog.h"
#include "bina/app/TypeLibrary.h"
#include "bina/document/Document.h"
#include "bina/document/DocumentFactory.h"
#include "bina/io/JsonDocumentExporter.h"
#include "bina/io/JsonDocumentImporter.h"
#include "bina/schema/DataType.h"
#include "bina/schema/Field.h"
#include "bina/schema/TypeRegistry.h"

namespace bina::app {

namespace {

const QStringList kFilters{QStringLiteral("*.json")};

// Os tipos customizados usados pelos campos de 'field', em qualquer nível, e os deles.
void collectTypes(const schema::TypeRegistry &types, const schema::Field &field, QSet<QString> &out)
{
    for (int i = 0; i < field.childCount(); ++i) {
        const schema::Field *child = field.child(i);
        const schema::Field *definition = types.definition(child->typeId());

        if (definition && !out.contains(child->typeId())) {
            out.insert(child->typeId());
            collectTypes(types, *definition, out);
        }

        collectTypes(types, *child, out);
    }
}

void collectFormats(const schema::Field &field, QSet<QString> &out)
{
    for (int i = 0; i < field.childCount(); ++i) {
        const schema::Field *child = field.child(i);

        if (!child->formatId().isEmpty())
            out.insert(child->formatId());

        collectFormats(*child, out);
    }
}

} // namespace

TypeLibrary::TypeLibrary(MessageLog &log, QObject *parent)
    : QObject{parent}
    , m_log{log}
{
}

void TypeLibrary::open(const QString &directory, const QString &defaults)
{
    m_directory = directory;
    m_defaults = defaults;

    if (!QDir(m_directory).exists()) {
        if (QDir().mkpath(m_directory))
            copyDefaults(false);
        else
            m_log.add(tr("Não foi possível criar a pasta da biblioteca de tipos: %1").arg(m_directory), true);
    }

    reload();
}

void TypeLibrary::reload()
{
    m_catalog.clear();

    const QDir dir(m_directory);

    for (const QFileInfo &file : dir.entryInfoList(kFilters, QDir::Files, QDir::Name)) {
        io::JsonDocumentImporter importer;
        document::DocumentDto dto;

        schema::OperationResult valid = importer.readFile(file.filePath(), dto);

        // Validado como uma estrutura: tipos desconhecidos, ciclos e formatos incompatíveis.
        if (valid) {
            document::Document probe;
            valid = document::DocumentFactory().create(dto, probe);
        }

        if (!valid) {
            m_log.add(tr("Biblioteca de tipos: '%1' ignorado: %2").arg(file.fileName(), valid.error()), true);
            continue;
        }

        m_catalog.add(dto);
    }

    emit changed();
}

schema::OperationResult TypeLibrary::save(const document::Document &document, const QString &id)
{
    using schema::OperationResult;

    const schema::TypeRegistry &types = *document.types();
    const schema::DataType *type = types.find(id);

    if (!type || !type->definition())
        return OperationResult::failure(tr("O tipo '%1' não é um tipo customizado").arg(id));

    if (m_directory.isEmpty() || !QDir().mkpath(m_directory))
        return OperationResult::failure(tr("A pasta da biblioteca de tipos não está disponível"));

    QSet<QString> usedTypes{id};
    collectTypes(types, *type->definition(), usedTypes);

    QSet<QString> usedFormats;
    for (const QString &typeId : std::as_const(usedTypes)) {
        const schema::DataType *used = types.find(typeId);

        if (!used->formatId().isEmpty())
            usedFormats.insert(used->formatId());

        collectFormats(*used->definition(), usedFormats);
    }

    // Na ordem do documento: cada tipo vem depois dos que ele usa.
    const document::DocumentDto all = document::DocumentFactory::toDto(document);
    document::DocumentDto dto;
    dto.maxAlign = all.maxAlign;
    dto.bigEndian = all.bigEndian;

    for (const document::DocumentFormatterDto &formatter : all.formatters)
        if (usedFormats.contains(formatter.id))
            dto.formatters << formatter;

    for (const document::DocumentTypeDto &t : all.types)
        if (usedTypes.contains(t.id))
            dto.types << t;

    const QString path = QDir(m_directory).filePath(id + QStringLiteral(".json"));

    if (const OperationResult written = io::JsonDocumentExporter().writeFile(path, dto); !written)
        return written;

    reload();

    return OperationResult::success();
}

int TypeLibrary::restoreDefaults()
{
    if (m_directory.isEmpty() || !QDir().mkpath(m_directory))
        return 0;

    const int copied = copyDefaults(true);

    reload();

    return copied;
}

int TypeLibrary::copyDefaults(bool overwrite)
{
    int copied = 0;

    for (const QFileInfo &file : QDir(m_defaults).entryInfoList(kFilters, QDir::Files, QDir::Name)) {
        const QString target = QDir(m_directory).filePath(file.fileName());

        if (QFile::exists(target)) {
            if (!overwrite)
                continue;

            QFile::remove(target);
        }

        if (!QFile::copy(file.filePath(), target)) {
            m_log.add(tr("Não foi possível copiar o tipo padrão '%1' para a biblioteca.").arg(file.fileName()), true);
            continue;
        }

        // A cópia de um recurso sai só de leitura.
        QFile::setPermissions(target, QFile::permissions(target) | QFileDevice::WriteOwner | QFileDevice::WriteUser);
        ++copied;
    }

    return copied;
}

} // namespace bina::app

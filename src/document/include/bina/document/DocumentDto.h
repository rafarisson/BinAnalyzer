#pragma once

#include <QList>
#include <QString>


namespace bina::document {

struct DocumentFieldDto
{
    QString name;
    QString type;
    QString formatter;
    int count = 1;
    QList<DocumentFieldDto> children;
};

// Padrão printf nomeado, referenciado pelo id em campos e tipos.
struct DocumentFormatterDto
{
    QString id;
    QString pattern;
};

struct DocumentTypeDto
{
    QString id;
    QString defaultFormatter;
    DocumentFieldDto definition;
};

struct DocumentDto
{
    static constexpr int CurrentVersion = 1;

    int version = CurrentVersion;
    int maxAlign = 4;
    bool bigEndian = false;
    QList<DocumentFormatterDto> formatters;
    QList<DocumentTypeDto> types;
    DocumentFieldDto root;
};

} // namespace bina::document

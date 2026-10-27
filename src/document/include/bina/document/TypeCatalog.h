#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

#include "bina/document/DocumentDto.h"

namespace bina::document {

// Tipos e formatos disponíveis fora do documento (a biblioteca de tipos), como DTOs: só leitura para
// o documento, que copia um tipo (com os tipos e formatos de que ele depende) quando um campo passa a
// usá-lo. Os ids são guardados normalizados.
class TypeCatalog
{
public:

    // Acrescenta os tipos e os formatos de 'data' (os campos da raiz são ignorados). Um id já
    // existente é substituído.
    void add(const DocumentDto &data);
    void clear();

    QStringList typeIds() const { return m_typeOrder; }
    bool contains(const QString &id) const { return m_types.contains(id); }

    // nullptr se o id não existe. Os ponteiros valem até a próxima alteração do catálogo.
    const DocumentTypeDto *type(const QString &id) const;
    const DocumentFormatterDto *formatter(const QString &id) const;

private:

    QHash<QString, DocumentTypeDto> m_types;
    QStringList m_typeOrder;
    QHash<QString, DocumentFormatterDto> m_formatters;
};

} // namespace bina::document

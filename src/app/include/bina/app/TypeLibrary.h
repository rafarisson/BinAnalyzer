#pragma once

#include <QObject>
#include <QString>

#include "bina/document/TypeCatalog.h"
#include "bina/schema/OperationResult.h"

namespace bina::document { class Document; }

namespace bina::app {

class MessageLog;

// A biblioteca de tipos: uma pasta com um JSON por tipo (no mesmo formato da estrutura, sem campos na
// raiz), lida para um document::TypeCatalog. Cada arquivo traz o tipo, os tipos de que ele depende e
// os formatos que usa, e é validado sozinho; um arquivo inválido é ignorado com um aviso no log.
//
// Na primeira vez (a pasta não existe) ela é criada com os tipos padrão, copiados de 'defaults' (os
// recursos ":/types" no executável). Depois disso a pasta é do usuário: um tipo padrão apagado não
// volta, a não ser por restoreDefaults().
class TypeLibrary : public QObject
{
    Q_OBJECT

public:

    // O log precisa sobreviver à biblioteca.
    explicit TypeLibrary(MessageLog &log, QObject *parent = nullptr);

    QString directory() const { return m_directory; }

    // Passa a usar 'directory' (criada com os tipos padrão se não existe) e lê os tipos.
    void open(const QString &directory, const QString &defaults = QStringLiteral(":/types"));

    // Lê de novo os arquivos da pasta. Avisa por changed().
    void reload();

    const document::TypeCatalog &catalog() const { return m_catalog; }
    bool contains(const QString &id) const { return m_catalog.contains(id); }

    // Grava o tipo 'id' do documento em "<id>.json", com os tipos e formatos de que ele depende,
    // substituindo o arquivo se já existe, e relê a pasta.
    schema::OperationResult save(const document::Document &document, const QString &id);

    // Copia os tipos padrão para a pasta, substituindo os de mesmo nome, e relê a pasta. Retorna
    // quantos arquivos foram copiados.
    int restoreDefaults();

signals:

    void changed();

private:

    int copyDefaults(bool overwrite);

    MessageLog &m_log;
    QString m_directory;
    QString m_defaults;
    document::TypeCatalog m_catalog;
};

} // namespace bina::app

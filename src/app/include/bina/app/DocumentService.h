#pragma once

#include <QObject>
#include <QString>

#include "bina/document/Document.h"
#include "bina/document/DocumentDto.h"
#include "bina/io/Codecs.h"

namespace bina::app {

class MessageLog;

// Abrir, criar e salvar a estrutura do documento, e o estado de alterações não salvas. O formato do
// arquivo vem da extensão (DocumentCodecRegistry); o resultado de cada comando vai para o MessageLog.
class DocumentService : public QObject
{
    Q_OBJECT

public:

    // O documento e o log precisam sobreviver ao serviço.
    DocumentService(document::Document &document, MessageLog &log, QObject *parent = nullptr);

    QString path() const { return m_path; }
    bool isDirty() const { return m_dirty; }

    bool newDocument();

    // Caminho local ou URL "file:". Um recurso (":/...") abre como modelo: sem caminho, então
    // salvar pede um arquivo novo.
    bool open(const QString &pathOrUrl);

    // Sem caminho (documento novo ou modelo) não salva: quem chama pede um com saveAs().
    bool save();
    bool saveAs(const QString &pathOrUrl);

    const io::DocumentCodecRegistry &codecs() const { return m_codecs; }

signals:

    void pathChanged();
    void dirtyChanged();

private:

    bool load(const document::DocumentDto &dto, const QString &path);
    void setDirty(bool dirty);
    void setPath(const QString &path);

    document::Document &m_document;
    MessageLog &m_log;
    io::DocumentCodecRegistry m_codecs{io::DocumentCodecRegistry::withBuiltins()};

    QString m_path;
    bool m_dirty{false};
};

} // namespace bina::app

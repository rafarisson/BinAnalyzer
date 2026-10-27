#pragma once

#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include "bina/app/DocumentService.h"
#include "bina/app/ExportService.h"
#include "bina/app/MessageLog.h"
#include "bina/document/Document.h"
#include "bina/model/ResultTableModel.h"
#include "bina/model/ResultTreeModel.h"
#include "bina/model/SchemaTreeModel.h"
#include "bina/presentation/FormatPatternController.h"
#include "bina/presentation/TypeDefinitionController.h"
#include "bina/session/AnalysisSession.h"

namespace bina::app {

// Fachada da aplicação para o QML: reúne o documento, a sessão de análise, os models e os
// serviços, e expõe os comandos (abrir e salvar a estrutura, abrir o binário, exportar,
// reanalisar) e o estado que a janela mostra. O trabalho fica nos serviços:
//   DocumentService  abrir, criar e salvar a estrutura; alterações não salvas
//   ExportService    exportar o resultado
//   MessageLog       mensagens para o usuário
class AppController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Criado pela aplicação")

    Q_PROPERTY(bina::model::SchemaTreeModel *schemaModel READ schemaModel CONSTANT)
    Q_PROPERTY(bina::model::ResultTreeModel *resultModel READ resultModel CONSTANT)
    Q_PROPERTY(bina::model::ResultTableModel *tableModel READ tableModel CONSTANT)
    Q_PROPERTY(bina::presentation::TypeDefinitionController *typeController READ typeController CONSTANT)
    Q_PROPERTY(bina::presentation::FormatPatternController *formatController READ formatController CONSTANT)
    Q_PROPERTY(bina::session::AnalysisSession *session READ session CONSTANT)
    Q_PROPERTY(bina::app::MessageLog *messageLog READ messageLog CONSTANT)

    Q_PROPERTY(QString schemaPath READ schemaPath NOTIFY schemaPathChanged)
    Q_PROPERTY(QString dataPath READ dataPath NOTIFY dataPathChanged)
    Q_PROPERTY(bool dirty READ isDirty NOTIFY dirtyChanged)
    Q_PROPERTY(QString windowTitle READ windowTitle NOTIFY titleChanged)

    // Opções do documento.
    Q_PROPERTY(bool bigEndian READ bigEndian WRITE setBigEndian NOTIFY optionsChanged)
    Q_PROPERTY(int maxAlign READ maxAlign WRITE setMaxAlign NOTIFY optionsChanged)

public:

    explicit AppController(QObject *parent = nullptr);

    model::SchemaTreeModel *schemaModel() { return &m_schemaModel; }
    model::ResultTreeModel *resultModel() { return &m_resultModel; }
    model::ResultTableModel *tableModel() { return &m_tableModel; }
    presentation::TypeDefinitionController *typeController() { return &m_typeController; }
    presentation::FormatPatternController *formatController() { return &m_formatController; }
    session::AnalysisSession *session() { return &m_session; }
    MessageLog *messageLog() { return &m_log; }
    const document::Document *document() const { return &m_document; }

    QString schemaPath() const { return m_documents.path(); }
    QString dataPath() const { return m_dataPath; }
    bool isDirty() const { return m_documents.isDirty(); }
    QString windowTitle() const;

    // As mensagens como exibidas ("[hh:mm:ss] texto"), da mais antiga à mais nova.
    QStringList messages() const { return m_log.lines(); }

    bool bigEndian() const { return m_document.byteOrder() == QSysInfo::BigEndian; }
    void setBigEndian(bool big);
    int maxAlign() const { return m_document.maxAlign(); }
    void setMaxAlign(int align);

    // Aceitam caminho local ou URL "file:" (como entregue pelos diálogos do QML).
    Q_INVOKABLE bool newSchema();
    Q_INVOKABLE bool openSchema(const QString &pathOrUrl);
    Q_INVOKABLE bool saveSchema();
    Q_INVOKABLE bool saveSchemaAs(const QString &pathOrUrl);
    Q_INVOKABLE bool openData(const QString &pathOrUrl);

    // Exporta o resultado da análise como está na tela; o formato vem da extensão (.json ou .csv,
    // e sem extensão vale .json).
    Q_INVOKABLE bool exportResult(const QString &pathOrUrl);

    Q_INVOKABLE void reanalyze();
    Q_INVOKABLE void clearMessages();

signals:

    void schemaPathChanged();
    void dataPathChanged();
    void dirtyChanged();
    void titleChanged();
    void optionsChanged();

    // Uma mensagem nova (para um aviso rápido na janela).
    void messageAdded(const QString &text, bool isError);

private:

    void connectMessages();

    // Declarados na ordem de dependência: cada um só usa os anteriores, e é destruído antes deles.
    MessageLog m_log;
    document::Document m_document;
    session::AnalysisSession m_session;
    model::SchemaTreeModel m_schemaModel;
    model::ResultTreeModel m_resultModel;
    model::ResultTableModel m_tableModel;
    presentation::TypeDefinitionController m_typeController;
    presentation::FormatPatternController m_formatController;
    DocumentService m_documents;
    ExportService m_exports;

    QString m_dataPath;
    QString m_lastSessionError;
    bool m_loadingData{false};
};

} // namespace bina::app

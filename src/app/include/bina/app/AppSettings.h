#pragma once

#include <QHash>
#include <QObject>
#include <QRect>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlEngine;

namespace bina::app {

// Preferências da pessoa, guardadas num arquivo INI (QSettings). Valem para o programa todo, não
// para uma estrutura: as opções do documento (ordem dos bytes, alinhamento) ficam no JSON dele.
//
//   [File]        version
//   [Appearance]  mode, accent
//   [Window]      x, y, width, height, maximized, schemaPaneWidth
//   [View]        schemaTab, tableMode, detailsExpanded
//   [Folders]     schema, data, export     (última pasta de cada diálogo de arquivo)
//   [RecentSchemas], [RecentData]          (arquivos recentes, o mais novo primeiro)
//
// O que é lido do arquivo é validado: um valor inválido (arquivo editado à mão ou de outra versão)
// volta ao padrão, sem quebrar o programa. Cada alteração é gravada na hora; sync() garante a
// gravação antes de encerrar.
//
// O QML usa a instância única (singleton); os testes criam a própria, com um arquivo temporário.
class AppSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Aparência: "system", "light" ou "dark", e uma das cores de destaque (acentIds()).
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeChanged)
    Q_PROPERTY(QString accentId READ accentId WRITE setAccentId NOTIFY themeChanged)

    // Janela: posição e tamanho na última vez em que estava na área normal (nulo se nunca gravados).
    Q_PROPERTY(QRect windowRect READ windowRect WRITE setWindowRect NOTIFY windowChanged)
    Q_PROPERTY(bool windowMaximized READ windowMaximized WRITE setWindowMaximized NOTIFY windowChanged)
    Q_PROPERTY(int schemaPaneWidth READ schemaPaneWidth WRITE setSchemaPaneWidth NOTIFY windowChanged)

    // Estado das vistas.
    Q_PROPERTY(int schemaTab READ schemaTab WRITE setSchemaTab NOTIFY viewChanged)
    Q_PROPERTY(bool tableMode READ tableMode WRITE setTableMode NOTIFY viewChanged)
    Q_PROPERTY(bool detailsExpanded READ detailsExpanded WRITE setDetailsExpanded NOTIFY viewChanged)

    // Última pasta de cada diálogo de arquivo (a pasta Documentos se não houver uma que exista).
    Q_PROPERTY(QUrl schemaFolder READ schemaFolder NOTIFY foldersChanged)
    Q_PROPERTY(QUrl dataFolder READ dataFolder NOTIFY foldersChanged)
    Q_PROPERTY(QUrl exportFolder READ exportFolder NOTIFY foldersChanged)

    // Arquivos recentes que ainda existem.
    Q_PROPERTY(QStringList recentSchemas READ recentSchemas NOTIFY recentChanged)
    Q_PROPERTY(QStringList recentData READ recentData NOTIFY recentChanged)

public:

    // Versão do formato do arquivo; aumenta quando uma chave muda de significado.
    static constexpr int kVersion = 1;
    static constexpr int kMaxRecent = 10;

    // Tamanho mínimo aceito para a janela lida do arquivo.
    static constexpr int kMinWindowWidth = 300;
    static constexpr int kMinWindowHeight = 200;

    static constexpr int kSchemaTabCount = 4;

    explicit AppSettings(const QString &filePath, QObject *parent = nullptr);

    // A instância do programa, no arquivo padrão (appDataDirectory()/settings.ini). O nome da
    // aplicação precisa estar definido antes da primeira chamada.
    static AppSettings *instance();
    static QString defaultFilePath();

    // Fábrica do singleton do QML.
    static AppSettings *create(QQmlEngine *, QJSEngine *);

    static QStringList accentIds();

    QString filePath() const { return m_settings.fileName(); }

    QString themeMode() const { return m_themeMode; }
    void setThemeMode(const QString &mode);
    QString accentId() const { return m_accentId; }
    void setAccentId(const QString &id);

    QRect windowRect() const { return m_windowRect; }
    void setWindowRect(const QRect &rect);
    bool windowMaximized() const { return m_windowMaximized; }
    void setWindowMaximized(bool maximized);
    int schemaPaneWidth() const { return m_schemaPaneWidth; }
    void setSchemaPaneWidth(int width);

    int schemaTab() const { return m_schemaTab; }
    void setSchemaTab(int tab);
    bool tableMode() const { return m_tableMode; }
    void setTableMode(bool table);
    bool detailsExpanded() const { return m_detailsExpanded; }
    void setDetailsExpanded(bool expanded);

    QUrl schemaFolder() const { return folder(QStringLiteral("schema")); }
    QUrl dataFolder() const { return folder(QStringLiteral("data")); }
    QUrl exportFolder() const { return folder(QStringLiteral("export")); }

    QStringList recentSchemas() const { return existing(m_recentSchemas); }
    QStringList recentData() const { return existing(m_recentData); }

    // Guarda a pasta do arquivo escolhido num diálogo ("schema", "data" ou "export"). Outros
    // nomes e URLs que não são de arquivo local são ignorados.
    Q_INVOKABLE void rememberFolder(const QString &kind, const QUrl &file);

    // Põe o arquivo no começo da lista de recentes. Recursos embutidos (":/...") e arquivos que
    // não existem não entram.
    Q_INVOKABLE void addRecentSchema(const QString &path);
    Q_INVOKABLE void addRecentData(const QString &path);
    Q_INVOKABLE void clearRecent();

    // Apaga o arquivo e volta tudo ao padrão.
    Q_INVOKABLE void resetToDefaults();

    // Grava o que estiver pendente.
    Q_INVOKABLE void sync() { m_settings.sync(); }

signals:

    void themeChanged();
    void windowChanged();
    void viewChanged();
    void foldersChanged();
    void recentChanged();

private:

    void load();
    void loadRecent(const QString &group, QStringList &out);
    void storeRecent(const QString &group, const QStringList &list);
    void addRecent(const QString &group, QStringList &list, const QString &path);

    QUrl folder(const QString &kind) const;
    static QStringList existing(const QStringList &paths);
    static bool validWindowRect(const QRect &rect);

    QSettings m_settings;

    QString m_themeMode;
    QString m_accentId;
    QRect m_windowRect;
    bool m_windowMaximized{false};
    int m_schemaPaneWidth{0};
    int m_schemaTab{0};
    bool m_tableMode{false};
    bool m_detailsExpanded{true};
    QHash<QString, QString> m_folders;
    QStringList m_recentSchemas;
    QStringList m_recentData;
};

} // namespace bina::app

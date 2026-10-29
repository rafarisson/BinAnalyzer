#include "TestSupport.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "bina/app/AppSettings.h"
#include "bina/app/Paths.h"

using bina::app::AppSettings;

// As preferências da pessoa: valores padrão, gravação e leitura, validação do que vem do arquivo,
// arquivos recentes e restauração. Cada teste usa um INI numa pasta temporária.
class tst_AppSettings : public QObject
{
    Q_OBJECT

private:

    QString path() const { return m_dir.filePath(QStringLiteral("settings.ini")); }

    // Cria um arquivo vazio na pasta temporária (para os testes dos recentes).
    QString touch(const QString &name) const
    {
        const QString file = m_dir.filePath(name);
        QFile out(file);
        return out.open(QIODevice::WriteOnly) ? file : QString();
    }

    void writeIni(const QByteArray &content) const
    {
        QFile out(path());
        QVERIFY(out.open(QIODevice::WriteOnly | QIODevice::Truncate));
        out.write(content);
    }

    QTemporaryDir m_dir;

private slots:

    void init()
    {
        QVERIFY(m_dir.isValid());
        QFile::remove(path());
    }

    void startsWithTheDefaults()
    {
        AppSettings settings(path());

        QCOMPARE(settings.themeMode(), QStringLiteral("system"));
        QCOMPARE(settings.accentId(), QStringLiteral("blue"));
        QVERIFY(settings.windowRect().isNull());
        QVERIFY(!settings.windowMaximized());
        QCOMPARE(settings.schemaPaneWidth(), 0);
        QCOMPARE(settings.schemaTab(), 0);
        QVERIFY(!settings.tableMode());
        QVERIFY(settings.detailsExpanded());
        QVERIFY(settings.recentSchemas().isEmpty());
        QVERIFY(settings.recentData().isEmpty());
        QVERIFY(settings.schemaFolder().isLocalFile());
    }

    void keepsWhatWasSetAcrossInstances()
    {
        {
            AppSettings settings(path());
            settings.setThemeMode(QStringLiteral("dark"));
            settings.setAccentId(QStringLiteral("purple"));
            settings.setWindowRect(QRect(10, 20, 1200, 700));
            settings.setWindowMaximized(true);
            settings.setSchemaPaneWidth(480);
            settings.setSchemaTab(2);
            settings.setTableMode(true);
            settings.setDetailsExpanded(false);
            settings.sync();
        }

        AppSettings reopened(path());
        QCOMPARE(reopened.themeMode(), QStringLiteral("dark"));
        QCOMPARE(reopened.accentId(), QStringLiteral("purple"));
        QCOMPARE(reopened.windowRect(), QRect(10, 20, 1200, 700));
        QVERIFY(reopened.windowMaximized());
        QCOMPARE(reopened.schemaPaneWidth(), 480);
        QCOMPARE(reopened.schemaTab(), 2);
        QVERIFY(reopened.tableMode());
        QVERIFY(!reopened.detailsExpanded());
    }

    void writesTheFileAsReadableIni()
    {
        AppSettings settings(path());
        settings.setThemeMode(QStringLiteral("light"));
        settings.sync();

        QFile in(path());
        QVERIFY(in.open(QIODevice::ReadOnly));
        const QString text = QString::fromUtf8(in.readAll());

        QVERIFY(text.contains(QStringLiteral("[Appearance]")));
        QVERIFY(text.contains(QStringLiteral("mode=light")));
        QVERIFY(text.contains(QStringLiteral("version=1")));
    }

    void emitsChangesOnlyWhenTheValueChanges()
    {
        AppSettings settings(path());
        QSignalSpy theme(&settings, &AppSettings::themeChanged);

        settings.setThemeMode(QStringLiteral("dark"));
        settings.setThemeMode(QStringLiteral("dark"));
        QCOMPARE(theme.size(), 1);
    }

    void rejectsInvalidValuesWhenSetting()
    {
        AppSettings settings(path());

        settings.setThemeMode(QStringLiteral("sepia"));
        settings.setAccentId(QStringLiteral("pink"));
        settings.setWindowRect(QRect(0, 0, 10, 10));
        settings.setSchemaTab(9);
        settings.setSchemaPaneWidth(-5);

        QCOMPARE(settings.themeMode(), QStringLiteral("system"));
        QCOMPARE(settings.accentId(), QStringLiteral("blue"));
        QVERIFY(settings.windowRect().isNull());
        QCOMPARE(settings.schemaTab(), 0);
        QCOMPARE(settings.schemaPaneWidth(), 0);
    }

    void fallsBackToDefaultsForInvalidFileContent()
    {
        writeIni("[Appearance]\n"
                 "mode=neon\n"
                 "accent=pink\n"
                 "[Window]\n"
                 "x=5\ny=5\nwidth=3\nheight=4\n"
                 "schemaPaneWidth=-1\n"
                 "[View]\n"
                 "schemaTab=77\n");

        AppSettings settings(path());

        QCOMPARE(settings.themeMode(), QStringLiteral("system"));
        QCOMPARE(settings.accentId(), QStringLiteral("blue"));
        QVERIFY(settings.windowRect().isNull());
        QCOMPARE(settings.schemaPaneWidth(), 0);
        QCOMPARE(settings.schemaTab(), 0);
    }

    void survivesGarbageInTheFile()
    {
        writeIni("isto não é um INI\n[[[\n=\n\x01\x02\n");

        AppSettings settings(path());
        QCOMPARE(settings.themeMode(), QStringLiteral("system"));
    }

    void remembersTheFolderOfEachDialog()
    {
        const QString sub = m_dir.filePath(QStringLiteral("dados"));
        QVERIFY(QDir().mkpath(sub));

        AppSettings settings(path());
        QSignalSpy folders(&settings, &AppSettings::foldersChanged);

        settings.rememberFolder(QStringLiteral("data"), QUrl::fromLocalFile(sub + QStringLiteral("/a.bin")));
        QCOMPARE(settings.dataFolder(), QUrl::fromLocalFile(sub));
        QCOMPARE(folders.size(), 1);

        // Os outros diálogos não mudam, e nomes e URLs inválidos são ignorados.
        QVERIFY(settings.schemaFolder() != QUrl::fromLocalFile(sub));
        settings.rememberFolder(QStringLiteral("other"), QUrl::fromLocalFile(sub + QStringLiteral("/a.bin")));
        settings.rememberFolder(QStringLiteral("schema"), QUrl(QStringLiteral("https://example.com/a.json")));
        QCOMPARE(folders.size(), 1);

        settings.sync();
        AppSettings reopened(path());
        QCOMPARE(reopened.dataFolder(), QUrl::fromLocalFile(sub));
    }

    void ignoresAFolderThatNoLongerExists()
    {
        {
            QTemporaryDir gone;
            QVERIFY(gone.isValid());
            AppSettings settings(path());
            settings.rememberFolder(QStringLiteral("export"), QUrl::fromLocalFile(gone.filePath(QStringLiteral("x.json"))));
            settings.sync();
        }

        AppSettings settings(path());
        QVERIFY(settings.exportFolder().isLocalFile());
        QVERIFY(QDir(settings.exportFolder().toLocalFile()).exists());
    }

    void recentFilesComeNewestFirstWithoutDuplicates()
    {
        const QString a = touch(QStringLiteral("a.json"));
        const QString b = touch(QStringLiteral("b.json"));
        QVERIFY(!a.isEmpty() && !b.isEmpty());

        AppSettings settings(path());
        settings.addRecentSchema(a);
        settings.addRecentSchema(b);
        settings.addRecentSchema(a);

        QCOMPARE(settings.recentSchemas(), (QStringList{QDir::cleanPath(a), QDir::cleanPath(b)}));
        QVERIFY(settings.recentData().isEmpty());
    }

    void recentFilesAreLimitedAndPersisted()
    {
        AppSettings settings(path());

        for (int i = 0; i < AppSettings::kMaxRecent + 3; ++i)
            settings.addRecentData(touch(QStringLiteral("d%1.bin").arg(i)));

        QCOMPARE(settings.recentData().size(), AppSettings::kMaxRecent);
        QVERIFY(settings.recentData().constFirst().endsWith(QStringLiteral("d12.bin")));

        settings.sync();
        AppSettings reopened(path());
        QCOMPARE(reopened.recentData(), settings.recentData());
    }

    void recentFilesSkipEmbeddedResourcesAndMissingFiles()
    {
        AppSettings settings(path());
        QSignalSpy recent(&settings, &AppSettings::recentChanged);

        settings.addRecentSchema(QStringLiteral(":/samples/elements.json"));
        settings.addRecentSchema(m_dir.filePath(QStringLiteral("nao-existe.json")));
        settings.addRecentSchema(QString());

        QVERIFY(settings.recentSchemas().isEmpty());
        QCOMPARE(recent.size(), 0);
    }

    void recentFilesDropFilesThatWereRemoved()
    {
        const QString a = touch(QStringLiteral("a.json"));
        const QString b = touch(QStringLiteral("b.json"));

        AppSettings settings(path());
        settings.addRecentSchema(a);
        settings.addRecentSchema(b);
        QVERIFY(QFile::remove(a));

        QCOMPARE(settings.recentSchemas(), (QStringList{QDir::cleanPath(b)}));
    }

    void clearsTheRecentFiles()
    {
        AppSettings settings(path());
        settings.addRecentSchema(touch(QStringLiteral("a.json")));
        settings.addRecentData(touch(QStringLiteral("a.bin")));

        settings.clearRecent();
        QVERIFY(settings.recentSchemas().isEmpty());
        QVERIFY(settings.recentData().isEmpty());

        settings.sync();
        AppSettings reopened(path());
        QVERIFY(reopened.recentSchemas().isEmpty());
    }

    void resetsEverythingToTheDefaults()
    {
        AppSettings settings(path());
        settings.setThemeMode(QStringLiteral("dark"));
        settings.setAccentId(QStringLiteral("green"));
        settings.setTableMode(true);
        settings.setWindowRect(QRect(0, 0, 900, 600));
        settings.addRecentSchema(touch(QStringLiteral("a.json")));

        QSignalSpy theme(&settings, &AppSettings::themeChanged);
        settings.resetToDefaults();

        QCOMPARE(settings.themeMode(), QStringLiteral("system"));
        QCOMPARE(settings.accentId(), QStringLiteral("blue"));
        QVERIFY(!settings.tableMode());
        QVERIFY(settings.windowRect().isNull());
        QVERIFY(settings.recentSchemas().isEmpty());
        QCOMPARE(theme.size(), 1);

        AppSettings reopened(path());
        QCOMPARE(reopened.themeMode(), QStringLiteral("system"));
        QVERIFY(reopened.recentSchemas().isEmpty());
    }

    void defaultFileIsInTheSameFolderAsTheTypeLibrary()
    {
        QCoreApplication::setApplicationName(QStringLiteral("BinAnalyzerTest"));

        QCOMPARE(AppSettings::defaultFilePath(), bina::app::appDataDirectory() + QStringLiteral("/settings.ini"));
        QVERIFY(bina::app::appDataDirectory().endsWith(QStringLiteral("BinAnalyzerTest")));
    }
};

QTEST_GUILESS_MAIN(tst_AppSettings)
#include "tst_appsettings.moc"

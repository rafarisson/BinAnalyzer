#include "bina/app/AppSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QQmlEngine>
#include <QStandardPaths>

#include "bina/app/Paths.h"

namespace bina::app {

namespace {

const QString kSystem = QStringLiteral("system");
const QString kLight = QStringLiteral("light");
const QString kDark = QStringLiteral("dark");

const QStringList kFolderKinds{QStringLiteral("schema"), QStringLiteral("data"), QStringLiteral("export")};

const QString kRecentSchemas = QStringLiteral("RecentSchemas");
const QString kRecentData = QStringLiteral("RecentData");

bool validMode(const QString &mode)
{
    return mode == kSystem || mode == kLight || mode == kDark;
}

// Não confia em números lidos do arquivo: fora desta faixa não são de uma janela de verdade.
bool plausible(int value)
{
    return value >= -100000 && value <= 100000;
}

} // namespace

AppSettings::AppSettings(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_settings(filePath, QSettings::IniFormat)
{
    load();
}

QString AppSettings::defaultFilePath()
{
    return appDataDirectory() + QStringLiteral("/settings.ini");
}

AppSettings *AppSettings::instance()
{
    static AppSettings *settings = new AppSettings(defaultFilePath(), QCoreApplication::instance());

    return settings;
}

AppSettings *AppSettings::create(QQmlEngine *, QJSEngine *)
{
    AppSettings *settings = instance();
    QJSEngine::setObjectOwnership(settings, QJSEngine::CppOwnership);
    return settings;
}

QStringList AppSettings::accentIds()
{
    // Os mesmos ids de Theme.accents (qml/style/Theme.qml).
    return {QStringLiteral("blue"), QStringLiteral("teal"), QStringLiteral("green"), QStringLiteral("orange"),
            QStringLiteral("purple")};
}

bool AppSettings::validWindowRect(const QRect &rect)
{
    return rect.width() >= kMinWindowWidth && rect.height() >= kMinWindowHeight && rect.width() <= 100000
        && rect.height() <= 100000 && plausible(rect.x()) && plausible(rect.y());
}

void AppSettings::load()
{
    m_themeMode = m_settings.value(QStringLiteral("Appearance/mode")).toString();
    if (!validMode(m_themeMode))
        m_themeMode = kSystem;

    m_accentId = m_settings.value(QStringLiteral("Appearance/accent")).toString();
    if (!accentIds().contains(m_accentId))
        m_accentId = accentIds().constFirst();

    const QRect rect(m_settings.value(QStringLiteral("Window/x")).toInt(),
                     m_settings.value(QStringLiteral("Window/y")).toInt(),
                     m_settings.value(QStringLiteral("Window/width")).toInt(),
                     m_settings.value(QStringLiteral("Window/height")).toInt());
    m_windowRect = validWindowRect(rect) ? rect : QRect();
    m_windowMaximized = m_settings.value(QStringLiteral("Window/maximized"), false).toBool();

    const int paneWidth = m_settings.value(QStringLiteral("Window/schemaPaneWidth"), 0).toInt();
    m_schemaPaneWidth = paneWidth >= kMinWindowWidth && paneWidth <= 100000 ? paneWidth : 0;

    const int tab = m_settings.value(QStringLiteral("View/schemaTab"), 0).toInt();
    m_schemaTab = tab >= 0 && tab < kSchemaTabCount ? tab : 0;
    m_tableMode = m_settings.value(QStringLiteral("View/tableMode"), false).toBool();
    m_detailsExpanded = m_settings.value(QStringLiteral("View/detailsExpanded"), true).toBool();

    for (const QString &kind : kFolderKinds) {
        const QString dir = m_settings.value(QStringLiteral("Folders/") + kind).toString();
        if (!dir.isEmpty())
            m_folders.insert(kind, dir);
    }

    loadRecent(kRecentSchemas, m_recentSchemas);
    loadRecent(kRecentData, m_recentData);

    if (m_settings.value(QStringLiteral("File/version")).toInt() < kVersion)
        m_settings.setValue(QStringLiteral("File/version"), kVersion);
}

void AppSettings::loadRecent(const QString &group, QStringList &out)
{
    out.clear();

    const int size = m_settings.beginReadArray(group);
    for (int i = 0; i < size && out.size() < kMaxRecent; ++i) {
        m_settings.setArrayIndex(i);
        const QString path = m_settings.value(QStringLiteral("path")).toString();
        if (!path.isEmpty() && !out.contains(path))
            out.append(path);
    }
    m_settings.endArray();
}

void AppSettings::storeRecent(const QString &group, const QStringList &list)
{
    // Remove antes: escrever um array menor deixaria as entradas antigas para trás.
    m_settings.remove(group);

    m_settings.beginWriteArray(group, static_cast<int>(list.size()));
    for (int i = 0; i < list.size(); ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue(QStringLiteral("path"), list.at(i));
    }
    m_settings.endArray();
}

void AppSettings::setThemeMode(const QString &mode)
{
    if (!validMode(mode) || mode == m_themeMode)
        return;

    m_themeMode = mode;
    m_settings.setValue(QStringLiteral("Appearance/mode"), mode);
    emit themeChanged();
}

void AppSettings::setAccentId(const QString &id)
{
    if (!accentIds().contains(id) || id == m_accentId)
        return;

    m_accentId = id;
    m_settings.setValue(QStringLiteral("Appearance/accent"), id);
    emit themeChanged();
}

void AppSettings::setWindowRect(const QRect &rect)
{
    if (!validWindowRect(rect) || rect == m_windowRect)
        return;

    m_windowRect = rect;
    m_settings.setValue(QStringLiteral("Window/x"), rect.x());
    m_settings.setValue(QStringLiteral("Window/y"), rect.y());
    m_settings.setValue(QStringLiteral("Window/width"), rect.width());
    m_settings.setValue(QStringLiteral("Window/height"), rect.height());
    emit windowChanged();
}

void AppSettings::setWindowMaximized(bool maximized)
{
    if (maximized == m_windowMaximized)
        return;

    m_windowMaximized = maximized;
    m_settings.setValue(QStringLiteral("Window/maximized"), maximized);
    emit windowChanged();
}

void AppSettings::setSchemaPaneWidth(int width)
{
    if (width < kMinWindowWidth || width > 100000 || width == m_schemaPaneWidth)
        return;

    m_schemaPaneWidth = width;
    m_settings.setValue(QStringLiteral("Window/schemaPaneWidth"), width);
    emit windowChanged();
}

void AppSettings::setSchemaTab(int tab)
{
    if (tab < 0 || tab >= kSchemaTabCount || tab == m_schemaTab)
        return;

    m_schemaTab = tab;
    m_settings.setValue(QStringLiteral("View/schemaTab"), tab);
    emit viewChanged();
}

void AppSettings::setTableMode(bool table)
{
    if (table == m_tableMode)
        return;

    m_tableMode = table;
    m_settings.setValue(QStringLiteral("View/tableMode"), table);
    emit viewChanged();
}

void AppSettings::setDetailsExpanded(bool expanded)
{
    if (expanded == m_detailsExpanded)
        return;

    m_detailsExpanded = expanded;
    m_settings.setValue(QStringLiteral("View/detailsExpanded"), expanded);
    emit viewChanged();
}

QUrl AppSettings::folder(const QString &kind) const
{
    const QString dir = m_folders.value(kind);

    if (!dir.isEmpty() && QDir(dir).exists())
        return QUrl::fromLocalFile(dir);

    return QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
}

void AppSettings::rememberFolder(const QString &kind, const QUrl &file)
{
    if (!kFolderKinds.contains(kind) || !file.isLocalFile())
        return;

    const QString dir = QFileInfo(file.toLocalFile()).absolutePath();
    if (dir.isEmpty() || dir == m_folders.value(kind))
        return;

    m_folders.insert(kind, dir);
    m_settings.setValue(QStringLiteral("Folders/") + kind, dir);
    emit foldersChanged();
}

QStringList AppSettings::existing(const QStringList &paths)
{
    QStringList result;

    for (const QString &path : paths) {
        if (QFileInfo::exists(path))
            result.append(path);
    }

    return result;
}

void AppSettings::addRecent(const QString &group, QStringList &list, const QString &path)
{
    // Recursos embutidos (os exemplos) e arquivos que não existem não são "recentes".
    if (path.isEmpty() || path.startsWith(QLatin1Char(':')) || !QFileInfo::exists(path))
        return;

    const QString clean = QDir::cleanPath(QFileInfo(path).absoluteFilePath());

    // O mesmo arquivo sobe para o começo, em vez de aparecer duas vezes.
    QStringList next{clean};
    for (const QString &item : std::as_const(list)) {
        if (next.size() >= kMaxRecent)
            break;
        if (QFileInfo::exists(item) && QFileInfo(item) != QFileInfo(clean))
            next.append(item);
    }

    if (next == list)
        return;

    list = next;
    storeRecent(group, list);
    emit recentChanged();
}

void AppSettings::addRecentSchema(const QString &path)
{
    addRecent(kRecentSchemas, m_recentSchemas, path);
}

void AppSettings::addRecentData(const QString &path)
{
    addRecent(kRecentData, m_recentData, path);
}

void AppSettings::clearRecent()
{
    if (m_recentSchemas.isEmpty() && m_recentData.isEmpty())
        return;

    m_recentSchemas.clear();
    m_recentData.clear();
    m_settings.remove(kRecentSchemas);
    m_settings.remove(kRecentData);
    emit recentChanged();
}

void AppSettings::resetToDefaults()
{
    m_settings.clear();
    m_settings.setValue(QStringLiteral("File/version"), kVersion);
    m_settings.sync();

    m_themeMode = kSystem;
    m_accentId = accentIds().constFirst();
    m_windowRect = QRect();
    m_windowMaximized = false;
    m_schemaPaneWidth = 0;
    m_schemaTab = 0;
    m_tableMode = false;
    m_detailsExpanded = true;
    m_folders.clear();
    m_recentSchemas.clear();
    m_recentData.clear();

    emit themeChanged();
    emit windowChanged();
    emit viewChanged();
    emit foldersChanged();
    emit recentChanged();
}

} // namespace bina::app

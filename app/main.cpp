#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>
#include <QTranslator>

#include "bina/app/AppController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("BinAnalyzer"));

    // Os textos estão em português; em outra língua do sistema usa a tradução, se houver.
    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("BinAnalyzer"), QStringLiteral("_"), QStringLiteral(":/i18n")))
        QCoreApplication::installTranslator(&translator);

    // Uso: BinAnalyzerApp [estrutura.json [dados.bin]]. Sem argumentos abre os exemplos embutidos
    // (samples/), como modelo: salvar pede um arquivo novo.
    const QStringList args = app.arguments();

    bina::app::AppController controller;

    // Antes de abrir a estrutura, para que os tipos da biblioteca já apareçam nos campos.
    controller.openTypeLibrary(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                               + QStringLiteral("/types"));

    if (args.size() > 1) {
        controller.openSchema(args.at(1));
        if (args.size() > 2)
            controller.openData(args.at(2));
    } else {
        controller.openSchema(QStringLiteral(":/samples/elements.json"));
        controller.openData(QStringLiteral(":/samples/elements.bin"));
    }

    QQmlApplicationEngine engine;

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.setInitialProperties({{QStringLiteral("app"), QVariant::fromValue(&controller)}});
    engine.loadFromModule("BinAnalyzer", "Main");

#ifdef BINA_UI_TEST_HOOKS
    // Para testes da interface: BINA_SCREENSHOT=arquivo.png salva a janela e encerra.
    if (const QString shot = qEnvironmentVariable("BINA_SCREENSHOT"); !shot.isEmpty() && !engine.rootObjects().isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QTimer::singleShot(2000, &app, [window, shot] {
            const bool saved = window && window->grabWindow().save(shot);
            QCoreApplication::exit(saved ? 0 : 1);
        });
    }
#endif

    return QGuiApplication::exec();
}

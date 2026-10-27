#include "TestSupport.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "bina/app/AppController.h"

using namespace TestSupport;
using bina::app::AppController;

// O controlador da aplicação: abrir e salvar, alterações não salvas, opções e mensagens.
class tst_AppController : public QObject
{
    Q_OBJECT

private slots:

    void opensTheSampleSchemaAndData()
    {
        AppController app;
        QSignalSpy titles(&app, &AppController::titleChanged);

        QVERIFY(app.openSchema(sample("elements.json")));
        QVERIFY(app.openData(sample("elements.bin")));

        QCOMPARE(app.schemaPath(), sample("elements.json"));
        QCOMPARE(app.dataPath(), sample("elements.bin"));
        QVERIFY(!app.isDirty());
        QVERIFY(app.windowTitle().contains("elements.json"));
        QVERIFY(!app.windowTitle().endsWith("*"));
        QVERIFY(titles.size() > 0);

        QTRY_COMPARE(app.session()->recordCount(), 1);
        QVERIFY(app.schemaModel()->rowCount() > 1);   // os campos da raiz
    }

    void acceptsFileUrls()
    {
        AppController app;

        QVERIFY(app.openSchema(QUrl::fromLocalFile(sample("elements.json")).toString()));
        QVERIFY(app.openData(QUrl::fromLocalFile(sample("elements4.bin")).toString()));
        QTRY_COMPARE(app.session()->recordCount(), 4);
    }

    void editingMakesTheDocumentDirty()
    {
        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));

        QSignalSpy dirty(&app, &AppController::dirtyChanged);

        app.setMaxAlign(2);

        QVERIFY(app.isDirty());
        QCOMPARE(dirty.size(), 1);
        QVERIFY(app.windowTitle().endsWith("*"));
        QCOMPARE(app.maxAlign(), 2);
    }

    void saveAndReopenKeepsTheStructureAndOptions()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath("saida.json");

        {
            AppController app;
            QVERIFY(app.openSchema(sample("elements.json")));
            app.setBigEndian(true);
            app.setMaxAlign(2);
            QVERIFY(app.isDirty());

            QVERIFY(app.saveSchemaAs(path));

            QVERIFY(!app.isDirty());
            QCOMPARE(app.schemaPath(), path);
        }

        AppController again;
        QVERIFY(again.openSchema(path));
        QVERIFY(again.bigEndian());
        QCOMPARE(again.maxAlign(), 2);
        QVERIFY(!again.isDirty());
    }

    void savingWithoutAPathNeedsSaveAs()
    {
        AppController app;
        QVERIFY(app.newSchema());
        QVERIFY(!app.saveSchema());                           // sem caminho: o QML abre "Salvar como"
    }

    void savingAddsTheJsonSuffix()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));
        QVERIFY(app.saveSchemaAs(dir.filePath("sem_sufixo")));
        QVERIFY(QFile::exists(dir.filePath("sem_sufixo.json")));
    }

    void saveWithAKnownPathOverwritesIt()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.filePath("a.json");
        QVERIFY(QFile::copy(sample("elements.json"), path));

        AppController app;
        QVERIFY(app.openSchema(path));
        app.setMaxAlign(8);
        QVERIFY(app.isDirty());

        QVERIFY(app.saveSchema());
        QVERIFY(!app.isDirty());

        AppController again;
        QVERIFY(again.openSchema(path));
        QCOMPARE(again.maxAlign(), 8);
    }

    void failuresBecomeMessages()
    {
        AppController app;
        QSignalSpy added(&app, &AppController::messageAdded);

        QVERIFY(!app.openSchema(sample("nao_existe.json")));
        QVERIFY(!app.openData(sample("nao_existe.bin")));

        QCOMPARE(added.size(), 2);
        QVERIFY(added.at(0).at(1).toBool());                  // é um erro
        QCOMPARE(app.messages().size(), 2);

        app.clearMessages();
        QVERIFY(app.messages().isEmpty());
    }

    void invalidStructureKeepsTheCurrentOne()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile bad(dir.filePath("ruim.json"));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write(R"({"version":1,"elements":[{"name":"a","type":"uint8"},{"name":"a","type":"uint8"}]})");
        bad.close();

        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));
        QVERIFY(!app.openSchema(bad.fileName()));

        QCOMPARE(app.schemaPath(), sample("elements.json"));
        QVERIFY(app.messages().last().contains("repetido"));
        QVERIFY(app.schemaModel()->rowCount() > 1);   // os campos da raiz
    }

    void formatModelListsTheCustomFormats()
    {
        AppController app;
        auto *formats = app.formatController();

        QVERIFY(formats->patternIds().isEmpty());          // documento novo, sem formatos
        QVERIFY(formats->currentSpec().isEmpty());

        QVERIFY(app.openSchema(sample("elements.json")));
        QVERIFY(formats->patternIds().contains("version_text"));
        QVERIFY(formats->patternIds().contains("point"));

        QSignalSpy changed(formats, &bina::presentation::FormatPatternController::currentPatternChanged);
        formats->setCurrentPattern("point");
        QCOMPARE(changed.size(), 1);
        QCOMPARE(formats->currentSpec(), QStringLiteral("(%(x)g, %(y)g, %(z)g)"));

        formats->setCurrentPattern("nao_existe");          // ignorado
        QCOMPARE(formats->currentPattern(), QStringLiteral("point"));

        const QVariantMap result = formats->preview(formats->currentSpec(), formats->suggestSample(formats->currentSpec()));
        QVERIFY(result.value("ok").toBool());
        QCOMPARE(result.value("text").toString(), QStringLiteral("(1.5, 1.5, 1.5)"));

        QVERIFY(app.newSchema());                          // outra estrutura: a lista é refeita
        QVERIFY(formats->patternIds().isEmpty());
    }

    void applyingAFormatMarksTheDataAsStale()
    {
        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));
        QVERIFY(app.openData(sample("elements.bin")));
        QTRY_VERIFY(!app.session()->isStale());
        QVERIFY(!app.isDirty());

        auto *formats = app.formatController();
        formats->setCurrentPattern("point");

        QSignalSpy added(&app, &AppController::messageAdded);
        QSignalSpy specChanged(formats, &bina::presentation::FormatPatternController::currentSpecChanged);

        QVERIFY(formats->applyPattern("<%(x)g|%(y)g|%(z)g>"));
        QCOMPARE(formats->currentSpec(), QStringLiteral("<%(x)g|%(y)g|%(z)g>"));
        QCOMPARE(specChanged.size(), 1);
        QVERIFY(app.session()->isStale());
        QVERIFY(app.isDirty());
        QCOMPARE(added.size(), 1);
        QVERIFY(!added.first().at(1).toBool());

        // O mesmo padrão de novo não muda nada.
        QVERIFY(formats->applyPattern("<%(x)g|%(y)g|%(z)g>"));
        QCOMPARE(added.size(), 1);

        // Padrão inválido: recusado, com a mensagem de erro, e o formato continua como estava.
        QVERIFY(!formats->applyPattern("%q"));
        QCOMPARE(formats->currentSpec(), QStringLiteral("<%(x)g|%(y)g|%(z)g>"));
        QCOMPARE(added.size(), 2);
        QVERIFY(added.last().at(1).toBool());

        app.reanalyze();
        QTRY_VERIFY(!app.session()->isStale());
    }

    void appliedFormatIsSaved()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));
        app.formatController()->setCurrentPattern("version_text");
        QVERIFY(app.formatController()->applyPattern("V%(major)d"));
        QVERIFY(app.saveSchemaAs(dir.filePath("novo.json")));

        AppController again;
        QVERIFY(again.openSchema(dir.filePath("novo.json")));
        again.formatController()->setCurrentPattern("version_text");
        QCOMPARE(again.formatController()->currentSpec(), QStringLiteral("V%(major)d"));
    }

    void newFormatAndTypeAreCreatedSelectedAndSaved()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));

        auto *formats = app.formatController();
        QVERIFY(formats->createPattern("meu_formato", "<%d>"));
        QCOMPARE(formats->currentPattern(), QStringLiteral("meu_formato"));
        QCOMPARE(formats->currentSpec(), QStringLiteral("<%d>"));
        QVERIFY(!formats->createPattern("meu_formato", "%d"));
        QVERIFY(!formats->createPattern("outro", "%("));

        auto *types = app.typeController();
        QVERIFY(types->createType("meu_tipo"));
        QCOMPARE(types->currentType(), QStringLiteral("meu_tipo"));
        QVERIFY(!types->isModified());
        QVERIFY(!types->createType("meu_tipo"));
        QVERIFY(app.isDirty());

        QVERIFY(app.saveSchemaAs(dir.filePath("novo.json")));

        AppController again;
        QVERIFY(again.openSchema(dir.filePath("novo.json")));
        QVERIFY(again.typeController()->typeIds().contains("meu_tipo"));
        QVERIFY(again.formatController()->patternIds().contains("meu_formato"));
    }

    void exportsTheResultAsJsonAndSpreadsheet()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        AppController app;
        QVERIFY(!app.exportResult(dir.filePath("vazio.json")));

        QVERIFY(app.openSchema(sample("elements.json")));
        QVERIFY(app.openData(sample("elements.bin")));

        const QString jsonPath = dir.filePath("saida.json");
        QVERIFY(app.exportResult(jsonPath));
        QFile json(jsonPath);
        QVERIFY(json.open(QIODevice::ReadOnly));
        const QJsonArray nodes = QJsonDocument::fromJson(json.readAll()).array();
        QVERIFY(!nodes.isEmpty());
        const QJsonObject first = nodes.first().toObject();
        for (const char *key : {"name", "type", "offset", "bytes", "value"})
            QVERIFY2(first.contains(QLatin1String(key)), key);

        // Textos saem sem as aspas que a view mostra.
        QVERIFY(json.seek(0));
        QVERIFY(!QString::fromUtf8(json.readAll()).contains("\\\""));

        const QString csvPath = dir.filePath("saida.csv");
        QVERIFY(app.exportResult(csvPath));
        QFile csv(csvPath);
        QVERIFY(csv.open(QIODevice::ReadOnly));
        QCOMPARE(csv.readAll().count("\r\n"), app.tableModel()->rowCount() + 1);

        QVERIFY(!app.exportResult(dir.filePath("saida.txt")));
    }

    void typeEditsStayInTheDraftUntilApplied()
    {
        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));
        QVERIFY(app.openData(sample("elements.bin")));
        QTRY_VERIFY(!app.session()->isStale());

        auto *types = app.typeController();
        QVERIFY(types->typeIds().contains("vec3"));
        types->setCurrentType("vec3");

        auto *draft = types->draftModel();
        const auto nameAt = [&](int row) { return draft->data(draft->index(row, 0), Qt::DisplayRole).toString(); };

        QCOMPARE(draft->rowCount(), 3);
        QCOMPARE(nameAt(0), QStringLiteral("x"));
        QVERIFY(!types->isModified());

        // Editar muda só o rascunho.
        QVERIFY(draft->setData(draft->index(0, 0), "px"));
        QVERIFY(types->isModified());
        QVERIFY(!app.isDirty());
        QVERIFY(!app.session()->isStale());
        QCOMPARE(app.document()->types()->definition("vec3")->child(0)->name(), QStringLiteral("x"));

        // Restaurar descarta.
        types->restore();
        QVERIFY(!types->isModified());
        QCOMPARE(nameAt(0), QStringLiteral("x"));

        // Aplicar grava no documento e pede nova análise.
        QSignalSpy added(&app, &AppController::messageAdded);
        QVERIFY(draft->setData(draft->index(0, 0), "px"));
        QVERIFY(draft->trigger(draft->index(2, 0), "insertBelow"));        // novo campo no fim
        QCOMPARE(draft->rowCount(), 4);
        QVERIFY(types->apply());

        QVERIFY(!types->isModified());
        QVERIFY(app.isDirty());
        QVERIFY(app.session()->isStale());
        QCOMPARE(added.size(), 1);
        QVERIFY(added.first().first().toString().contains("vec3"));
        QCOMPARE(app.document()->types()->definition("vec3")->child(0)->name(), QStringLiteral("px"));
        QCOMPARE(app.document()->types()->definition("vec3")->childCount(), 4);
        QCOMPARE(types->currentType(), QStringLiteral("vec3"));

        app.reanalyze();
        QTRY_VERIFY(!app.session()->isStale());
    }

    void typeDraftFollowsTheStructureRules()
    {
        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));

        auto *types = app.typeController();
        types->setCurrentType("vec3");
        auto *draft = types->draftModel();

        QSignalSpy added(&app, &AppController::messageAdded);

        // Um tipo não pode conter a si mesmo, nem por outro tipo que o usa ("bone" usa "vec3").
        QVERIFY(!draft->setData(draft->index(0, bina::model::SchemaTreeModel::TypeIdColumn), "vec3"));
        QVERIFY(!draft->setData(draft->index(0, bina::model::SchemaTreeModel::TypeIdColumn), "bone"));
        QCOMPARE(added.size(), 2);

        // Sem campos a definição é recusada, e o documento fica como estava.
        for (int i = 0; i < 3; ++i)
            QVERIFY(draft->trigger(draft->index(0, 0), "remove"));
        QCOMPARE(draft->rowCount(), 0);
        QVERIFY(!types->apply());
        QCOMPARE(added.size(), 3);
        QVERIFY(added.last().at(1).toBool());
        QCOMPARE(app.document()->types()->definition("vec3")->childCount(), 3);

        types->restore();
        QCOMPARE(draft->rowCount(), 3);

        // Trocar de tipo descarta o que não foi aplicado.
        QVERIFY(draft->setData(draft->index(0, 0), "px"));
        QVERIFY(types->isModified());
        types->setCurrentType("bone");
        QVERIFY(!types->isModified());
    }

    void rejectedEditsBecomeMessages()
    {
        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));

        QSignalSpy added(&app, &AppController::messageAdded);

        const QModelIndex magic = app.schemaModel()->index(0, 0);
        QVERIFY(!app.schemaModel()->setData(magic, "version"));          // nome repetido

        QCOMPARE(added.size(), 1);
        QVERIFY(added.first().at(1).toBool());
    }

    void newSchemaStartsEmptyAndUntitled()
    {
        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));
        QVERIFY(app.newSchema());

        QVERIFY(app.schemaPath().isEmpty());
        QVERIFY(!app.isDirty());
        QVERIFY(app.windowTitle().contains("Sem título"));
        QCOMPARE(app.schemaModel()->rowCount(), 0);

        // Dá para começar a montar a estrutura pelo menu do model.
        QVERIFY(app.schemaModel()->trigger({}, "insertInside"));
        QVERIFY(app.isDirty());
    }

    void optionsMakeLoadedDataStale()
    {
        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));
        QVERIFY(app.openData(sample("elements.bin")));
        QTRY_VERIFY(!app.session()->isStale());

        QSignalSpy options(&app, &AppController::optionsChanged);
        app.setBigEndian(true);

        QVERIFY(app.bigEndian());
        QVERIFY(options.size() > 0);
        QVERIFY(app.session()->isStale());

        app.reanalyze();
        QVERIFY(!app.session()->isStale());
    }

    void unusableAlignmentIsAMessage()
    {
        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));
        QSignalSpy added(&app, &AppController::messageAdded);

        app.setMaxAlign(3);

        QCOMPARE(app.maxAlign(), 4);
        QCOMPARE(added.size(), 1);
        QVERIFY(!app.isDirty());
    }

    void messageLogIsAListModelWithALimit()
    {
        AppController app;
        bina::app::MessageLog *log = app.messageLog();
        app.clearMessages();

        log->add("aviso");
        log->add("erro", true);

        QCOMPARE(log->rowCount(), 2);
        QCOMPARE(log->data(log->index(1), bina::app::MessageLog::TextRole).toString(), QStringLiteral("erro"));
        QVERIFY(log->data(log->index(1), bina::app::MessageLog::IsErrorRole).toBool());
        QVERIFY(log->data(log->index(0)).toString().endsWith("] aviso"));

        for (int i = 0; i < bina::app::MessageLog::kMaxMessages; ++i)
            log->add(QString::number(i));

        QCOMPARE(log->count(), bina::app::MessageLog::kMaxMessages);
        QCOMPARE(log->text(log->count() - 1), QString::number(bina::app::MessageLog::kMaxMessages - 1));
    }

    void savingWithoutSuffixDoesNotOverwriteAnExistingFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile existing(dir.filePath("estrutura.json"));
        QVERIFY(existing.open(QIODevice::WriteOnly));
        existing.write("conteúdo anterior");
        existing.close();

        AppController app;
        QVERIFY(app.openSchema(sample("elements.json")));

        // O diálogo confirmou "estrutura", não "estrutura.json": não substitui sem perguntar.
        QVERIFY(!app.saveSchemaAs(dir.filePath("estrutura")));
        QVERIFY(app.messages().last().contains("já existe"));
        QCOMPARE(readFile(existing.fileName()), QByteArray("conteúdo anterior"));

        QVERIFY(app.saveSchemaAs(dir.filePath("estrutura.json")));     // com a extensão, substitui
        QVERIFY(readFile(existing.fileName()) != QByteArray("conteúdo anterior"));
    }

    void unknownFileFormatsAreRefused()
    {
        AppController app;

        QVERIFY(!app.openSchema(sample("elements.yaml")));
        QVERIFY(app.messages().last().contains("formato desconhecido"));

        const bina::io::DocumentCodecRegistry codecs = bina::io::DocumentCodecRegistry::withBuiltins();
        QVERIFY(codecs.forPath("A.JSON"));
        QVERIFY(!codecs.forPath("a.yaml"));
        QCOMPARE(codecs.defaultSuffix(), QStringLiteral("json"));
    }

    void aResourceOpensAsATemplate()
    {
        AppController app;

        QVERIFY(app.openSchema(":/samples/elements.json"));
        QVERIFY(app.schemaPath().isEmpty());                            // salvar pede um arquivo novo
        QVERIFY(!app.saveSchema());
        QVERIFY(app.schemaModel()->rowCount() > 1);

        QVERIFY(app.openData(":/samples/elements.bin"));
        QTRY_COMPARE(app.session()->recordCount(), 1);
    }

    void exportIncludesTheRecordsNotYetRead()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile schema(dir.filePath("byte.json"));
        QVERIFY(schema.open(QIODevice::WriteOnly));
        schema.write(R"({"version":1,"elements":[{"name":"v","type":"uint8"}]})");
        schema.close();

        const qint64 total = bina::session::AnalysisSession::kRecordBatch + 50;
        QFile data(dir.filePath("dados.bin"));
        QVERIFY(data.open(QIODevice::WriteOnly));
        data.write(QByteArray(total, '\x07'));
        data.close();

        AppController app;
        QVERIFY(app.openSchema(schema.fileName()));
        QVERIFY(app.openData(data.fileName()));
        QTRY_COMPARE(app.session()->recordCount(), total);
        QCOMPARE(app.tableModel()->rowCount(), int(bina::session::AnalysisSession::kRecordBatch));

        const QString csv = dir.filePath("tabela.csv");
        QVERIFY(app.exportResult(csv));

        const QList<QByteArray> lines = readFile(csv).trimmed().split('\n');
        QCOMPARE(lines.size(), total + 1);                              // cabeçalho + registros
    }
};

QTEST_GUILESS_MAIN(tst_AppController)
#include "tst_appcontroller.moc"

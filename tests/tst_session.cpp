#include "TestSupport.h"

#include "bina/document/DocumentFactory.h"
#include "bina/model/EditorRoles.h"
#include "bina/model/ResultTreeModel.h"
#include "bina/model/ResultTableModel.h"
#include "bina/session/AnalysisSession.h"

using namespace TestSupport;
using bina::session::AnalysisSession;

namespace {

struct Fixture
{
    std::unique_ptr<bina::document::Document> doc{sampleDocument()};
    AnalysisSession session;

    Fixture() { session.setDocument(doc.get()); }

    bool load(const QString &name)
    {
        return session.loadFile(sample(name));
    }

    bina::schema::Field *field(const QString &path) const { return findField(doc->fields()->root(), path); }
};

}

// A sessão de análise e os models que exibem o resultado.
class tst_Session : public QObject
{
    Q_OBJECT

private slots:

    void loadingAnalyzesRightAway()
    {
        Fixture f;
        QSignalSpy analyzed(&f.session, &AnalysisSession::analyzed);

        QVERIFY(!f.session.hasData());
        QVERIFY(f.load("elements4.bin"));

        QCOMPARE(analyzed.size(), 1);
        QVERIFY(f.session.hasData());
        QVERIFY(!f.session.isStale());
        QCOMPARE(f.session.recordCount(), 4);
        QCOMPARE(f.session.completeRecords(), 4);

        const bina::parse::ParseResult result = f.session.takeResult();
        QCOMPARE(result.recordCount(), qint64(4));
        QCOMPARE(f.session.takeResult().recordCount(), qint64(0));      // entregue uma vez
    }

    void failedLoadReportsTheError()
    {
        Fixture f;

        QVERIFY(!f.session.loadFile(sample("nao_existe.bin")));
        QVERIFY(!f.session.error().isEmpty());
        QCOMPARE(f.session.statusText(), f.session.error());
        QVERIFY(!f.session.hasData());
    }

    void editingMarksStaleWithoutReanalyzing()
    {
        Fixture f;
        QVERIFY(f.load("elements.bin"));
        f.session.takeResult();

        QSignalSpy analyzed(&f.session, &AnalysisSession::analyzed);
        QSignalSpy invalidated(&f.session, &AnalysisSession::invalidated);

        QVERIFY(f.doc->fields()->setCount(f.field("magic"), 8));

        QVERIFY(f.session.isStale());
        QCOMPARE(analyzed.size(), 0);                                // não reanalisa sozinha
        QCOMPARE(invalidated.size(), 0);                             // o resultado antigo continua válido
        QCOMPARE(f.session.recordCount(), 1);

        f.session.reanalyze();

        QVERIFY(!f.session.isStale());
        QCOMPARE(analyzed.size(), 1);
        QVERIFY(f.session.takeResult().incompleteBytes() > 0);          // 196 bytes < 200 da nova estrutura
    }

    void removingAFieldInvalidatesTheResult()
    {
        Fixture f;
        QVERIFY(f.load("elements.bin"));
        f.session.takeResult();

        QSignalSpy invalidated(&f.session, &AnalysisSession::invalidated);

        QVERIFY(f.doc->fields()->removeField(f.field("checksum")));

        QCOMPARE(invalidated.size(), 1);
        QVERIFY(f.session.isStale());
        QVERIFY(f.session.hasData());
        QCOMPARE(f.session.recordCount(), 0);
        QVERIFY(f.session.statusText().contains("descartado"));

        // Sem o último campo o registro passa a ter 192 bytes: 196 bytes são 1 registro e mais 4 bytes.
        f.session.reanalyze();
        QCOMPARE(f.session.recordCount(), 2);
        QCOMPARE(f.session.completeRecords(), 1);
        QVERIFY(!f.session.isStale());
    }

    void autoPadChangesDoNotInvalidate()
    {
        Fixture f;
        QVERIFY(f.load("elements.bin"));
        f.session.takeResult();

        QSignalSpy invalidated(&f.session, &AnalysisSession::invalidated);

        // Trocar o tipo cria/remove auto_pad, mas nenhum campo do usuário é removido.
        QVERIFY(f.doc->fields()->setTypeId(f.field("header.bone_count"), "uint8"));

        QCOMPARE(invalidated.size(), 0);
        QVERIFY(f.session.isStale());
    }

    void optionsMarkTheResultStale()
    {
        Fixture f;
        QVERIFY(f.load("elements.bin"));
        f.session.takeResult();

        f.doc->setByteOrder(QSysInfo::BigEndian);
        QVERIFY(f.session.isStale());

        f.session.reanalyze();
        const bina::parse::ParseResult result = f.session.takeResult();
        QCOMPARE(result.records.front().children.at(1).children.at(2).value.toUnsigned(), quint64(11265));
    }

    void aNewDocumentIsAnalyzedAutomatically()
    {
        Fixture f;
        QVERIFY(f.load("elements.bin"));
        f.session.takeResult();

        QSignalSpy invalidated(&f.session, &AnalysisSession::invalidated);
        QSignalSpy analyzed(&f.session, &AnalysisSession::analyzed);

        bina::io::JsonDocumentImporter importer;
        bina::document::DocumentDto dto;
        QVERIFY(importer.readFile(sample("elements.json"), dto));

        bina::document::DocumentFactory factory;
        QVERIFY(factory.create(dto, *f.doc));

        QCOMPARE(invalidated.size(), 1);
        QTRY_COMPARE(analyzed.size(), 1);
        QVERIFY(!f.session.isStale());
    }

    void statusTexts()
    {
        Fixture f;

        QCOMPARE(f.session.statusText(), QStringLiteral("Nenhum arquivo carregado"));

        f.session.setBytes(readFile(sample("elements.bin")));
        QCOMPARE(f.session.statusText(), QStringLiteral("1 registro (196 bytes)"));

        f.session.setBytes(readFile(sample("elements4.bin")));
        QCOMPARE(f.session.statusText(), QStringLiteral("4 registros de 196 bytes (784 bytes)"));

        f.session.setBytes(readFile(sample("elements4.bin")).left(686));
        QCOMPARE(f.session.statusText(), QStringLiteral("3 registros completos + 1 incompleto (98 de 196 bytes)"));

        f.session.setBytes(readFile(sample("elements.bin")).left(10));
        QCOMPARE(f.session.statusText(), QStringLiteral("Registro incompleto: 10 de 196 bytes"));

        f.session.setBytes(QByteArray());
        QCOMPARE(f.session.statusText(), QStringLiteral("Arquivo vazio (registro de 196 bytes)"));

        f.session.clear();
        QCOMPARE(f.session.statusText(), QStringLiteral("Nenhum arquivo carregado"));
    }

    void resultModelUpdatesInPlaceWhenTheShapeIsTheSame()
    {
        Fixture f;
        bina::model::ResultTreeModel model;
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
        model.setSession(&f.session);

        QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
        QSignalSpy changes(&model, &QAbstractItemModel::dataChanged);

        QVERIFY(f.load("elements.bin"));
        QCOMPARE(resets.size(), 1);

        const QModelIndex magic = model.index(0, bina::model::ResultTreeModel::ValueColumn);
        QCOMPARE(model.data(magic).toString(), QStringLiteral("\"BONE\""));

        // Mesma estrutura, outro conteúdo: atualiza no lugar, sem recriar as linhas.
        QPersistentModelIndex kept(model.index(2, 0));
        QVERIFY(f.session.loadFile(sample("elements.bin")));
        QCOMPARE(resets.size(), 1);
        QVERIFY(changes.size() > 0);
        QVERIFY(kept.isValid());

        // Outra quantidade de registros: as linhas são recriadas.
        QVERIFY(f.load("elements4.bin"));
        QCOMPARE(resets.size(), 2);
        QCOMPARE(model.rowCount(), 4);
        QCOMPARE(model.data(model.index(0, 0)).toString(), QStringLiteral("registro[0]"));
    }

    void resultModelFormatsAndRemembersTheChosenFormat()
    {
        Fixture f;
        bina::model::ResultTreeModel model;
        model.setSession(&f.session);
        QVERIFY(f.load("elements.bin"));

        auto indexOf = [&](const QString &name, int column) {
            for (int r = 0; r < model.rowCount(); ++r)
                if (model.data(model.index(r, 0)).toString() == name)
                    return model.index(r, column);
            return QModelIndex();
        };

        const QModelIndex checksumFormat = indexOf("checksum", bina::model::ResultTreeModel::FormatColumn);
        const QModelIndex checksumValue = indexOf("checksum", bina::model::ResultTreeModel::ValueColumn);
        QCOMPARE(model.data(checksumValue).toString(), QStringLiteral("0xDEADBEEF"));

        QVERIFY(model.compatibleFormats(checksumFormat).contains("dec"));
        QVERIFY(model.setData(checksumFormat, "dec"));
        QCOMPARE(model.data(checksumValue).toString(), QString::number(0xDEADBEEF));

        QVERIFY(!model.setData(checksumFormat, "float"));            // incompatível
        QVERIFY(model.setData(checksumFormat, ""));                  // volta ao formato do campo
        QCOMPARE(model.data(checksumValue).toString(), QStringLiteral("0xDEADBEEF"));

        // O seletor de formato vem das roles do model.
        QCOMPARE(model.data(checksumFormat, bina::model::EditorKindRole).toString(), QStringLiteral("combo"));
        QCOMPARE(model.data(checksumFormat, bina::model::ChoicesRole).toList().first().toMap().value("value").toString(), QString());
        QVERIFY(model.data(indexOf("checksum", 0), bina::model::EditorKindRole).toString().isEmpty());

        // Escolhas são esquecidas quando o resultado é descartado.
        QVERIFY(model.setData(checksumFormat, "dec"));
        f.session.clear();
        QVERIFY(f.load("elements.bin"));
        QCOMPARE(model.data(indexOf("checksum", bina::model::ResultTreeModel::ValueColumn)).toString(), QStringLiteral("0xDEADBEEF"));
    }

    void staleValuesStayReadableAfterATypeChange()
    {
        Fixture f;
        bina::model::ResultTreeModel model;
        model.setSession(&f.session);
        QVERIFY(f.load("elements.bin"));

        // checksum passa a float na estrutura, mas o valor lido era inteiro: como o formato do campo
        // não serve mais ao valor, ele continua legível no formato padrão do tipo original.
        QVERIFY(f.doc->fields()->setTypeId(f.field("checksum"), "float"));
        QVERIFY(f.session.isStale());

        bool found = false;
        for (int r = 0; r < model.rowCount(); ++r) {
            if (model.data(model.index(r, 0)).toString() == "checksum") {
                QCOMPARE(model.data(model.index(r, bina::model::ResultTreeModel::ValueColumn)).toString(), QString::number(0xDEADBEEF));
                found = true;
            }
        }
        QVERIFY(found);
    }

    void tableModelHasOneRowPerRecordAndOneColumnPerField()
    {
        Fixture f;
        bina::model::ResultTreeModel tree;
        bina::model::ResultTableModel table;
        QAbstractItemModelTester tester(&table, QAbstractItemModelTester::FailureReportingMode::Fatal);

        tree.setSession(&f.session);
        table.setSource(&tree);
        QVERIFY(f.load("elements4.bin"));

        QCOMPARE(table.rowCount(), 4);

        QStringList headers;
        for (int c = 0; c < table.columnCount(); ++c)
            headers << table.headerData(c, Qt::Horizontal).toString();

        QVERIFY(headers.contains("magic"));
        QVERIFY(headers.contains("version"));                         // struct com formato: uma coluna
        QVERIFY(headers.contains("header.flags"));
        QVERIFY(headers.contains("header.bounds.extra[1].weight"));   // grupos expandidos
        QVERIFY(headers.contains("bones[2].position"));
        QCOMPARE(table.headerData(3, Qt::Vertical).toString(), QStringLiteral("registro[3]"));

        const int versionColumn = int(headers.indexOf("version"));
        QCOMPARE(table.data(table.index(1, versionColumn)).toString(), QStringLiteral("v1.1.301"));
        QCOMPARE(table.data(table.index(3, versionColumn)).toString(), QStringLiteral("v1.3.303"));

        // Trocar o formato pela tabela vale para o campo (e portanto para todos os registros).
        const int flagsColumn = int(headers.indexOf("header.flags"));
        QVERIFY(table.compatibleFormats(table.index(0, flagsColumn)).contains("hex"));
        QVERIFY(table.setFormat(table.index(0, flagsColumn), "hex"));
        QCOMPARE(table.data(table.index(0, flagsColumn)).toString(), QStringLiteral("0x05"));
        QCOMPARE(table.data(table.index(2, flagsColumn)).toString(), QStringLiteral("0x14"));
    }

    void tableModelShowsMissingFieldsOfAnIncompleteRecordAsEmpty()
    {
        Fixture f;
        bina::model::ResultTreeModel tree;
        bina::model::ResultTableModel table;

        tree.setSession(&f.session);
        table.setSource(&tree);
        QVERIFY(f.session.loadFile(sample("elements4.bin")));
        f.session.setBytes(readFile(sample("elements4.bin")).left(686));

        QCOMPARE(table.rowCount(), 4);

        const int checksumColumn = table.columnCount() - 1;
        QVERIFY(table.data(table.index(3, checksumColumn), bina::model::ResultTableModel::TruncatedRole).toBool());
        QVERIFY(!table.data(table.index(0, checksumColumn), bina::model::ResultTableModel::TruncatedRole).toBool());
    }

    void largeFilesAreReadInBatchesByTheModels()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[{"name":"v","type":"uint8"}]})");
        QVERIFY(doc);

        AnalysisSession session;
        bina::model::ResultTreeModel tree;
        bina::model::ResultTableModel table;
        QAbstractItemModelTester treeTester(&tree, QAbstractItemModelTester::FailureReportingMode::Fatal);
        QAbstractItemModelTester tableTester(&table, QAbstractItemModelTester::FailureReportingMode::Fatal);

        tree.setSession(&session);
        table.setSource(&tree);
        session.setDocument(doc.get());

        const qint64 total = 2 * AnalysisSession::kRecordBatch + 10;
        session.setBytes(QByteArray(total, 'x'));

        // A sessão conta o arquivo todo; os models só têm o primeiro lote.
        QCOMPARE(session.recordCount(), total);
        QCOMPARE(tree.rowCount(), int(AnalysisSession::kRecordBatch));
        QCOMPARE(table.rowCount(), int(AnalysisSession::kRecordBatch));
        QVERIFY(tree.canFetchMore({}));

        table.fetchMore({});                                           // a tabela pede pela árvore
        QCOMPARE(tree.rowCount(), int(2 * AnalysisSession::kRecordBatch));
        QCOMPARE(table.rowCount(), tree.rowCount());

        tree.fetchAll();
        QCOMPARE(tree.rowCount(), int(total));
        QCOMPARE(table.rowCount(), int(total));
        QVERIFY(!tree.canFetchMore({}));
        QCOMPARE(tree.index(int(total) - 1, 0).data().toString(), QStringLiteral("registro[%1]").arg(total - 1));
    }

    void aStaleSessionReadsNoMoreRecords()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[{"name":"v","type":"uint8"}]})");
        QVERIFY(doc);

        AnalysisSession session;
        bina::model::ResultTreeModel tree;
        tree.setSession(&session);
        session.setDocument(doc.get());
        session.setBytes(QByteArray(AnalysisSession::kRecordBatch + 5, 'x'));
        QVERIFY(tree.canFetchMore({}));

        // Os registros seguintes teriam outra estrutura: só depois de analisar de novo.
        QVERIFY(doc->fields()->setName(findField(doc->fields()->root(), "v"), "w"));
        QVERIFY(session.isStale());
        QVERIFY(!tree.canFetchMore({}));

        session.reanalyze();
        QVERIFY(tree.canFetchMore({}));
    }

    void changingTheMaxAlignMarksTheResultStale()
    {
        // Sem campos com padding a mudar, nenhum campo é avisado: a sessão escuta o documento.
        auto doc = documentFromJson(R"({"version":1,"elements":[{"name":"v","type":"uint8"}]})");
        QVERIFY(doc);

        AnalysisSession session;
        session.setDocument(doc.get());
        session.setBytes(QByteArray(4, 'x'));
        QVERIFY(!session.isStale());

        QVERIFY(doc->setMaxAlign(8));
        QVERIFY(session.isStale());
    }

    void aFailedLoadKeepsThePreviousData()
    {
        Fixture f;
        QVERIFY(f.load("elements4.bin"));
        QCOMPARE(f.session.recordCount(), 4);

        QVERIFY(!f.session.loadFile(sample("nao_existe.bin")));
        QCOMPARE(f.session.recordCount(), 4);                          // os dados anteriores ficam
    }
};

QTEST_GUILESS_MAIN(tst_Session)
#include "tst_session.moc"

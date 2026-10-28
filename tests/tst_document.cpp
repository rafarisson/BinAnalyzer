#include "TestSupport.h"

#include "bina/document/TypeCatalog.h"
#include "bina/io/JsonDocumentExporter.h"

using namespace TestSupport;

namespace {

// O documento como JSON; vazio se a exportação falhou.
QByteArray toJson(const bina::document::Document &doc)
{
    QByteArray json;
    return bina::io::JsonDocumentExporter().serialize(bina::document::DocumentFactory::toDto(doc), json) ? json : QByteArray();
}

// Uma biblioteca de tipos com o conteúdo de um JSON de estrutura.
bina::document::TypeCatalog catalogFromJson(const QByteArray &json)
{
    bina::document::DocumentDto dto;
    bina::io::JsonDocumentImporter().parse(json, dto);

    bina::document::TypeCatalog catalog;
    catalog.add(dto);

    return catalog;
}

const QByteArray kGroupJson = R"({"version":1,"elements":[
    {"name":"g","elements":[{"name":"a","type":"uint8"},{"name":"b","type":"uint32"}]},
    {"name":"c","type":"uint8"}]})";

}

// Carregar, validar, editar e salvar a estrutura.
class tst_Document : public QObject
{
    Q_OBJECT

private slots:

    void jsonRoundTripIsStable()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        const QByteArray first = toJson(*doc);
        QVERIFY(!first.isEmpty());

        auto again = documentFromJson(first);
        QVERIFY(again);

        QCOMPARE(toJson(*again), first);
        QCOMPARE(childNames(again->fields()->root()), childNames(doc->fields()->root()));
    }

    void autoPadsAreNotSerialized()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        QVERIFY(!padSizes(findField(doc->fields()->root(), "header")).isEmpty());

        const QByteArray json = toJson(*doc);
        QVERIFY(!json.contains("auto_pad"));
    }

    void optionsArePersisted()
    {
        auto doc = documentFromJson(R"({"version":1,"maxAlign":2,"byteOrder":"big","elements":[
            {"name":"a","type":"uint8"}]})");
        QVERIFY(doc);

        QCOMPARE(doc->maxAlign(), 2);
        QCOMPARE(doc->byteOrder(), QSysInfo::BigEndian);

        const QByteArray json = toJson(*doc);
        QVERIFY(json.contains("\"byteOrder\": \"big\""));
        QVERIFY(json.contains("\"maxAlign\": 2"));
    }

    void rejectsInvalidDocuments_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("message");

        QTest::newRow("duplicate field")
            << QByteArray(R"({"version":1,"elements":[{"name":"a","type":"uint8"},{"name":"a","type":"uint8"}]})") << "repetido";
        QTest::newRow("duplicate in group")
            << QByteArray(R"({"version":1,"elements":[{"name":"g","elements":[
                {"name":"x","type":"uint8"},{"name":"x","type":"uint8"}]}]})") << "repetido";
        QTest::newRow("duplicate field in a type")
            << QByteArray(R"({"version":1,"types":[{"id":"t","elements":[
                {"name":"x","type":"uint8"},{"name":"x","type":"uint8"}]}],"elements":[{"name":"a","type":"t"}]})") << "repetido";
        QTest::newRow("unknown formatter")
            << QByteArray(R"({"version":1,"elements":[{"name":"a","type":"uint8","formatter":"nope"}]})") << "incompatível";
        QTest::newRow("incompatible formatter")
            << QByteArray(R"({"version":1,"elements":[{"name":"a","type":"uint8","formatter":"float"}]})") << "incompatível";
        QTest::newRow("newer version")
            << QByteArray(R"({"version":99,"elements":[]})") << "mais nova";
        QTest::newRow("invalid json")
            << QByteArray("{ isto nao e json") << QString();
        QTest::newRow("struct type without children")
            << QByteArray(R"({"version":1,"elements":[{"name":"g","type":"struct"}]})") << "struct";
        QTest::newRow("type with children")
            << QByteArray(R"({"version":1,"elements":[{"name":"g","type":"uint8","elements":[{"name":"x","type":"uint8"}]}]})") << "struct";
        QTest::newRow("zero count")
            << QByteArray(R"({"version":1,"elements":[{"name":"a","type":"uint8","count":0}]})") << "count";
        QTest::newRow("invalid max align")
            << QByteArray(R"({"version":1,"maxAlign":3,"elements":[{"name":"a","type":"uint8"}]})") << "potência de 2";
        QTest::newRow("bad byte order")
            << QByteArray(R"({"version":1,"byteOrder":"middle","elements":[]})") << "byteOrder";
    }

    void rejectsInvalidDocuments()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, message);

        QString error;
        QVERIFY(!documentFromJson(json, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY2(error.contains(message, Qt::CaseInsensitive), qPrintable(error));
    }

    void failedLoadLeavesTheDocumentUntouched()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::io::JsonDocumentImporter importer;
        bina::document::DocumentDto bad;
        QVERIFY(importer.parse(R"({"version":1,"elements":[{"name":"a","type":"nao_existe"}]})", bad));

        bina::document::DocumentFactory factory;
        QVERIFY(!factory.create(bad, *doc));

        QCOMPARE(childNames(doc->fields()->root()), QStringList({"magic", "version", "header", "bones", "checksum"}));
        QVERIFY(doc->types()->contains("vec3"));
    }

    void unicodeNamesArePreserved()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[{"name":"média","type":"uint8"}]})");
        QVERIFY(doc);

        bina::schema::Field *root = doc->fields()->root();
        QCOMPARE(childNames(root), QStringList({"média"}));

        bina::schema::Field *field = findField(root, "média");
        QVERIFY(field);
        QVERIFY(doc->fields()->setName(field, "ação nº 2"));
        QCOMPARE(field->name(), QStringLiteral("ação_nº_2"));    // espaço vira "_"; letras acentuadas ficam
    }

    void rejectedEditsAreAnnounced()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::document::FieldTree *tree = doc->fields();
        bina::schema::Field *root = tree->root();
        bina::schema::Field *magic = findField(root, "magic");
        QSignalSpy rejected(tree, &bina::document::FieldTree::editRejected);

        QVERIFY(!tree->setName(magic, "version"));          // já existe entre os irmãos
        QCOMPARE(rejected.size(), 1);
        QVERIFY(!tree->setName(magic, "   "));              // vazio
        QCOMPARE(rejected.size(), 2);
        QVERIFY(!tree->setCount(magic, 0));
        QCOMPARE(rejected.size(), 3);
        QVERIFY(!tree->setTypeId(magic, "nao_existe"));
        QCOMPARE(rejected.size(), 4);
        QVERIFY(!tree->setTypeId(magic, "struct"));         // struct só existe com filhos
        QCOMPARE(rejected.size(), 5);
        QVERIFY(!tree->setTypeId(findField(root, "header"), "uint8"));   // grupo com campos
        QCOMPARE(rejected.size(), 6);
        QVERIFY(!tree->setFormatId(findField(root, "checksum"), "float"));
        QCOMPARE(rejected.size(), 7);

        // Sem mudança não é recusa.
        QVERIFY(!tree->setName(magic, "magic"));
        QCOMPARE(rejected.size(), 7);

        QCOMPARE(magic->name(), QStringLiteral("magic"));
        QVERIFY(tree->setFormatId(findField(root, "checksum"), "bin"));
        QVERIFY(tree->setFormatId(findField(root, "checksum"), ""));        // volta ao padrão
    }

    void autoPadCannotBeEdited()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::document::FieldTree *tree = doc->fields();
        bina::schema::Field *header = findField(tree->root(), "header");
        bina::schema::Field *pad = nullptr;

        for (int i = 0; i < header->childCount() && !pad; ++i)
            if (header->child(i)->isAutoPad())
                pad = header->child(i);

        QVERIFY(pad);
        QVERIFY(pad->isAutoPad());
        QVERIFY(!tree->setName(pad, "x"));
        QVERIFY(!tree->setCount(pad, 9));
        QVERIFY(!tree->setTypeId(pad, "uint32"));
        QVERIFY(!tree->setFormatId(pad, "hex"));
        QVERIFY(!tree->removeField(pad));
        QVERIFY(!tree->remove(header, pad->row(), 1));
        QVERIFY(!tree->canDrop(pad, findField(tree->root(), "magic"), bina::document::FieldTree::Onto));
        QVERIFY(!tree->canDrop(findField(tree->root(), "magic"), pad, bina::document::FieldTree::Onto));
        QVERIFY(!tree->addAfter(pad));
        QVERIFY(!tree->addInside(pad));
    }

    void userFieldNamedLikeThePadIsNotTreatedAsOne()
    {
        // O auto_pad é identificado por uma marca explícita, não pelo nome.
        auto doc = documentFromJson(R"({"version":1,"elements":[{"name":"a","type":"uint8"}]})");
        QVERIFY(doc);

        bina::schema::Field field;
        QCOMPARE(field.setName("auto_pad"), bina::schema::Field::Edit::Changed);
        QVERIFY(!field.isAutoPad());
    }

    void revisionChangesOnlyWithTheListOfFields()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::document::FieldTree *tree = doc->fields();
        const quint32 start = tree->revision();

        tree->setName(findField(tree->root(), "magic"), "assinatura");
        tree->setFormatId(findField(tree->root(), "checksum"), "bin");
        QCOMPARE(tree->revision(), start);

        QVERIFY(tree->insertField(tree->root(), 0));
        QVERIFY(tree->revision() > start);
    }

    void insertedFieldsGetUniqueNames()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::document::FieldTree *tree = doc->fields();
        bina::schema::Field *a = tree->insertField(tree->root(), 0);
        bina::schema::Field *b = tree->insertField(tree->root(), 0);

        QVERIFY(a && b);
        QCOMPARE(a->name(), QStringLiteral("field_1"));
        QCOMPARE(b->name(), QStringLiteral("field_2"));
    }

    void removingTheLastChildTurnsTheGroupIntoAField()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[
            {"name":"g","elements":[{"name":"x","type":"uint16"}]}]})");
        QVERIFY(doc);

        bina::document::FieldTree *tree = doc->fields();
        bina::schema::Field *group = findField(tree->root(), "g");
        QCOMPARE(group->typeId(), QStringLiteral("struct"));

        QVERIFY(tree->removeField(findField(group, "x")));
        QCOMPARE(group->childCount(), 0);                    // os auto_pad também saíram
        QCOMPARE(group->typeId(), QStringLiteral("uint32"));
    }

    void aGroupBecomesATypeWithTheSameLayout()
    {
        auto doc = documentFromJson(kGroupJson);
        QVERIFY(doc);

        bina::document::FieldTree *tree = doc->fields();
        bina::schema::Field *group = findField(tree->root(), "g");
        const int groupSize = tree->sizeInBytes(group);
        const int rootSize = tree->sizeInBytes(tree->root());
        QSignalSpy added(doc.get(), &bina::document::Document::typeAdded);

        QVERIFY(tree->canCollapseToType(group));
        QVERIFY(doc->extractType(group, "pair"));

        QCOMPARE(group->typeId(), QString("pair"));
        QCOMPARE(group->childCount(), 0);
        QCOMPARE(childNames(doc->types()->definition("pair")), QStringList({"a", "b"}));
        QCOMPARE(tree->sizeInBytes(group), groupSize);
        QCOMPARE(tree->sizeInBytes(tree->root()), rootSize);
        QCOMPARE(added.size(), 1);
        QVERIFY(tree->typeIds().contains("pair"));
    }

    void extractingRefusesAnIdInUse()
    {
        auto doc = documentFromJson(kGroupJson);
        QVERIFY(doc);

        bina::schema::Field *group = findField(doc->fields()->root(), "g");

        QVERIFY(!doc->extractType(group, "uint8"));
        QCOMPARE(childNames(group), QStringList({"a", "b"}));

        // Um campo sem filhos não vira tipo.
        QVERIFY(!doc->extractType(findField(doc->fields()->root(), "c"), "single"));
        QVERIFY(!doc->types()->contains("single"));
    }

    void aTypeBecomesAGroupAgain()
    {
        auto doc = documentFromJson(kGroupJson);
        QVERIFY(doc);

        bina::document::FieldTree *tree = doc->fields();
        bina::schema::Field *group = findField(tree->root(), "g");
        const int size = tree->sizeInBytes(group);
        QVERIFY(doc->extractType(group, "pair"));

        QVERIFY(tree->canInlineType(group));
        QVERIFY(tree->inlineType(group));

        QCOMPARE(group->typeId(), QString("struct"));
        QCOMPARE(childNames(group), QStringList({"a", "b"}));
        QCOMPARE(padSizes(group), QList<int>({3}));
        QCOMPARE(tree->sizeInBytes(group), size);

        // O tipo continua no documento, agora sem uso.
        QVERIFY(doc->types()->definition("pair"));
    }

    void onlyUnusedTypesCanBeRemoved()
    {
        auto doc = documentFromJson(kGroupJson);
        QVERIFY(doc);

        bina::schema::Field *group = findField(doc->fields()->root(), "g");
        QVERIFY(doc->extractType(group, "pair"));
        QVERIFY(doc->addType("spare"));

        QVERIFY(!doc->removeType("pair"));        // usado pela estrutura
        QVERIFY(!doc->removeType("uint8"));       // primitivo

        QSignalSpy removed(doc.get(), &bina::document::Document::typeRemoved);
        QVERIFY(doc->removeType("spare"));
        QCOMPARE(removed.size(), 1);
        QVERIFY(!doc->types()->contains("spare"));
        QVERIFY(!toJson(*doc).contains("spare"));
    }

    void aTypeUsedByAnotherTypeIsNotRemoved()
    {
        auto doc = documentFromJson(R"({"version":1,
            "types":[{"id":"inner","elements":[{"name":"x","type":"uint8"}]},
                     {"id":"outer","elements":[{"name":"i","type":"inner"}]}],
            "elements":[{"name":"a","type":"uint8"}]})");
        QVERIFY(doc);

        QVERIFY(!doc->removeType("inner"));
        QVERIFY(doc->removeType("outer"));
        QVERIFY(doc->removeType("inner"));
    }

    void aLibraryTypeIsCopiedWhenUsed()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[{"name":"when","type":"uint32"}]})");
        QVERIFY(doc);

        const bina::document::TypeCatalog library = catalogFromJson(readFile(sample("struct_tm.json")));
        doc->setLibrary(&library);

        QVERIFY(doc->fields()->typeIds().contains("struct_tm"));
        QVERIFY(!doc->types()->contains("struct_tm"));

        QSignalSpy imported(doc.get(), &bina::document::Document::typeImported);
        bina::schema::Field *when = findField(doc->fields()->root(), "when");

        QVERIFY(doc->fields()->setTypeId(when, "struct_tm"));

        QCOMPARE(imported.size(), 1);
        QVERIFY(doc->types()->definition("struct_tm"));
        QVERIFY(doc->formats()->contains("tm_datetime"));
        QCOMPARE(doc->types()->find("struct_tm")->formatId(), QString("tm_datetime"));
        QCOMPARE(doc->fields()->sizeInBytes(when), 36);

        // O documento salvo leva o tipo e o formato, e abre sem a biblioteca.
        auto reopened = documentFromJson(toJson(*doc));
        QVERIFY(reopened);
        QVERIFY(reopened->types()->definition("struct_tm"));
    }

    void aCopyBringsTheTypesItDependsOn()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[{"name":"a","type":"uint8"}]})");
        QVERIFY(doc);

        const bina::document::TypeCatalog library = catalogFromJson(R"({"version":1,
            "formatters":[{"id":"hexpair","pattern":"%(x)02x"}],
            "types":[{"id":"inner","elements":[{"name":"x","type":"uint8"}],"formatter":"hexpair"},
                     {"id":"outer","elements":[{"name":"i","type":"inner"},{"name":"j","type":"inner"}]}]})");
        doc->setLibrary(&library);

        QVERIFY(doc->fields()->setTypeId(findField(doc->fields()->root(), "a"), "outer"));

        QVERIFY(doc->types()->definition("inner"));
        QVERIFY(doc->types()->definition("outer"));
        QVERIFY(doc->formats()->contains("hexpair"));
    }

    void theDocumentTypeWinsOverTheLibrary()
    {
        auto doc = documentFromJson(R"({"version":1,
            "types":[{"id":"struct_tm","elements":[{"name":"tm_sec","type":"int32"}]}],
            "elements":[{"name":"a","type":"uint8"}]})");
        QVERIFY(doc);

        const bina::document::TypeCatalog library = catalogFromJson(readFile(sample("struct_tm.json")));
        doc->setLibrary(&library);

        using State = bina::document::Document::LibraryState;
        QCOMPARE(doc->libraryState("struct_tm"), State::DiffersFromLibrary);

        // Usar o tipo não traz o da biblioteca: o do documento continua.
        QVERIFY(doc->fields()->setTypeId(findField(doc->fields()->root(), "a"), "struct_tm"));
        QCOMPARE(doc->types()->definition("struct_tm")->childCount(), 1);

        QVERIFY(doc->updateTypeFromLibrary("struct_tm"));
        QCOMPARE(childNames(doc->types()->definition("struct_tm")).size(), 9);
        QCOMPARE(doc->libraryState("struct_tm"), State::DiffersFromLibrary);   // o formato padrão não muda
        QCOMPARE(doc->libraryState("uint8"), State::NotInLibrary);
    }

    void headerSurvivesTheJsonRoundTrip()
    {
        const QByteArray json = R"({"version":1,"types":[
            {"id":"hdr","header":true,"elements":[{"name":"magic","type":"uint32"}]},
            {"id":"plain","elements":[{"name":"a","type":"uint8"}]}],
            "header":"hdr","elements":[{"name":"v","type":"uint16"}]})";

        auto doc = documentFromJson(json);
        QVERIFY(doc);

        QCOMPARE(doc->headerIds(), QStringList{QStringLiteral("hdr")});
        QVERIFY(doc->isHeader(QStringLiteral("hdr")));
        QVERIFY(!doc->isHeader(QStringLiteral("plain")));
        QCOMPARE(doc->activeHeader(), QStringLiteral("hdr"));

        auto again = documentFromJson(toJson(*doc));
        QVERIFY(again);
        QCOMPARE(again->headerIds(), doc->headerIds());
        QCOMPARE(again->activeHeader(), QStringLiteral("hdr"));
    }

    void activeHeaderMustBeAHeader()
    {
        QString error;
        auto doc = documentFromJson(R"({"version":1,"types":[
            {"id":"plain","elements":[{"name":"a","type":"uint8"}]}],
            "header":"plain","elements":[{"name":"v","type":"uint16"}]})", &error);

        QVERIFY(!doc);
        QVERIFY(!error.isEmpty());
    }

    void headerCanBeAddedActivatedAndRemoved()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        QSignalSpy active(doc.get(), &bina::document::Document::activeHeaderChanged);

        QVERIFY(doc->addHeader(QStringLiteral("hdr")));
        QVERIFY(doc->isHeader(QStringLiteral("hdr")));
        QVERIFY(!doc->setActiveHeader(QStringLiteral("struct_tm")));   // não é um cabeçalho
        QVERIFY(doc->setActiveHeader(QStringLiteral("hdr")));
        QCOMPARE(active.size(), 1);
        QVERIFY(doc->activeHeaderDefinition());

        QVERIFY(doc->removeType(QStringLiteral("hdr")));
        QVERIFY(doc->activeHeader().isEmpty());
        QVERIFY(!doc->isHeader(QStringLiteral("hdr")));
        QCOMPARE(active.size(), 2);
    }

    void libraryHeadersAreKeptApartFromTypes()
    {
        const bina::document::TypeCatalog library = catalogFromJson(R"({"version":1,"types":[
            {"id":"hdr","header":true,"elements":[{"name":"magic","type":"uint32"}]},
            {"id":"plain","elements":[{"name":"a","type":"uint8"}]}],"elements":[]})");

        bina::document::Document doc;
        doc.setLibrary(&library);

        QCOMPARE(doc.availableLibraryHeaderIds(), QStringList{QStringLiteral("hdr")});
        QCOMPARE(doc.availableLibraryTypeIds(), QStringList{QStringLiteral("plain")});

        QVERIFY(doc.importType(QStringLiteral("hdr")));
        QVERIFY(doc.isHeader(QStringLiteral("hdr")));
        QVERIFY(doc.availableLibraryHeaderIds().isEmpty());
    }

    void theSameTypeIsReportedAsSame()
    {
        auto doc = sampleDocument("struct_tm.json");
        QVERIFY(doc);

        const bina::document::TypeCatalog library = catalogFromJson(readFile(sample("struct_tm.json")));
        doc->setLibrary(&library);

        QCOMPARE(doc->libraryState("struct_tm"), bina::document::Document::LibraryState::SameAsLibrary);
        QVERIFY(!doc->fields()->typeIds().isEmpty());
    }
};

QTEST_GUILESS_MAIN(tst_Document)
#include "tst_document.moc"

#include "TestSupport.h"

#include "bina/io/JsonDocumentExporter.h"

using namespace TestSupport;

namespace {

// O documento como JSON; vazio se a exportação falhou.
QByteArray toJson(const bina::document::Document &doc)
{
    QByteArray json;
    return bina::io::JsonDocumentExporter().serialize(bina::document::DocumentFactory::toDto(doc), json) ? json : QByteArray();
}

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
};

QTEST_GUILESS_MAIN(tst_Document)
#include "tst_document.moc"

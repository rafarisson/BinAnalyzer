#include "TestSupport.h"

#include "bina/format/FormatRegistry.h"
#include "bina/parse/BinaryParser.h"

using namespace TestSupport;
using bina::parse::ParsedNode;

namespace {

const ParsedNode *child(const ParsedNode &node, const QString &name)
{
    for (const ParsedNode &c : node.children)
        if (c.name == name)
            return &c;

    return nullptr;
}

// Nó por caminho de nomes a partir de um registro ("header.bounds.min").
const ParsedNode *find(const ParsedNode &record, const QString &path)
{
    const ParsedNode *node = &record;

    for (const QString &name : path.split(QLatin1Char('.')))
        node = node ? child(*node, name) : nullptr;

    return node;
}

}

// Leitura do binário segundo a estrutura: registros, offsets, valores, truncamento.
class tst_Parser : public QObject
{
    Q_OBJECT

private slots:

    void oneRecord()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::parse::BinaryParser parser(doc->types(), doc->byteOrder(), doc->maxAlign());
        bina::parse::ParseResult result;
        QVERIFY(parser.parse(doc->fields()->root(), readFile(sample("elements.bin")), result));

        QCOMPARE(result.recordCount(), qint64(1));
        QCOMPARE(result.recordSize, qint64(196));
        QCOMPARE(result.dataSize, qint64(196));
        QCOMPARE(result.incompleteBytes(), qint64(0));

        const ParsedNode &record = result.records.front();

        QCOMPARE(find(record, "magic")->offset, qint64(0));
        QCOMPARE(find(record, "magic")->value.text(), QStringLiteral("BONE"));
        QCOMPARE(find(record, "version.patch")->offset, qint64(6));
        QCOMPARE(find(record, "version.patch")->value.toUnsigned(), quint64(300));
        QCOMPARE(find(record, "header")->offset, qint64(8));
        QCOMPARE(find(record, "header.bounds")->offset, qint64(12));
        QCOMPARE(find(record, "header.bounds.max.z")->value.toDouble(), 6.5);
        QCOMPARE(find(record, "bones")->offset, qint64(60));
        QCOMPARE(find(record, "checksum")->offset, qint64(192));
        QCOMPARE(find(record, "checksum")->value.toUnsigned(), quint64(0xDEADBEEF));
    }

    void arraysOfStructsHaveOneNodePerInstance()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::parse::BinaryParser parser(doc->types(), doc->byteOrder(), doc->maxAlign());
        bina::parse::ParseResult result;
        QVERIFY(parser.parse(doc->fields()->root(), readFile(sample("elements.bin")), result));

        const ParsedNode *bones = find(result.records.front(), "bones");
        QVERIFY(bones);
        QCOMPARE(bones->children.size(), 3);
        QCOMPARE(bones->children.at(1).name, QStringLiteral("bones[1]"));
        QCOMPARE(bones->children.at(1).instance, 1);
        QCOMPARE(bones->children.at(1).offset, qint64(60 + 44));
        QCOMPARE(find(bones->children.at(2), "name")->value.text(), QStringLiteral("bone2"));

        // O alvo de formatação das instâncias é um único elemento.
        QCOMPARE(bones->children.at(1).target.count, 1);
        QCOMPARE(bones->target.count, 3);
    }

    void severalRecords()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::parse::BinaryParser parser(doc->types(), doc->byteOrder(), doc->maxAlign());
        bina::parse::ParseResult result;
        QVERIFY(parser.parse(doc->fields()->root(), readFile(sample("elements4.bin")), result));

        QCOMPARE(result.recordCount(), qint64(4));
        QCOMPARE(result.completeRecords(), qint64(4));
        QCOMPARE(result.records.at(3).offset, qint64(3 * 196));
        QCOMPARE(find(result.records.at(2), "version.minor")->value.toUnsigned(), quint64(2));
        QCOMPARE(result.records.at(3).name, QStringLiteral("record[3]"));
    }

    void incompleteLastRecordIsTruncated()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::parse::BinaryParser parser(doc->types(), doc->byteOrder(), doc->maxAlign());
        bina::parse::ParseResult result;
        QVERIFY(parser.parse(doc->fields()->root(), readFile(sample("elements4.bin")).left(686), result));

        QCOMPARE(result.recordCount(), qint64(4));
        QCOMPARE(result.completeRecords(), qint64(3));
        QCOMPARE(result.incompleteBytes(), qint64(98));

        QVERIFY(!result.records.at(2).truncated);
        QVERIFY(result.records.at(3).truncated);
        QVERIFY(!find(result.records.at(3), "magic")->truncated);
        QVERIFY(find(result.records.at(3), "checksum")->truncated);
        QVERIFY(!find(result.records.at(3), "checksum")->value.isValid());
    }

    void emptyDataGivesOneTruncatedRecord()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::parse::BinaryParser parser(doc->types(), doc->byteOrder(), doc->maxAlign());
        bina::parse::ParseResult result;
        QVERIFY(parser.parse(doc->fields()->root(), QByteArray(), result));

        QCOMPARE(result.recordCount(), qint64(1));
        QVERIFY(result.records.front().truncated);
    }

    void byteOrderChangesTheValues()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::parse::ParseResult little, big;
        QVERIFY(bina::parse::BinaryParser(doc->types(), QSysInfo::LittleEndian, doc->maxAlign()).parse(doc->fields()->root(), readFile(sample("elements.bin")), little));
        QVERIFY(bina::parse::BinaryParser(doc->types(), QSysInfo::BigEndian, doc->maxAlign()).parse(doc->fields()->root(), readFile(sample("elements.bin")), big));

        QCOMPARE(find(little.records.front(), "version.patch")->value.toUnsigned(), quint64(300));
        QCOMPARE(find(big.records.front(), "version.patch")->value.toUnsigned(), quint64(11265));
    }

    void valuesAreFormattedWithTheDocumentFormats()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::parse::BinaryParser parser(doc->types(), doc->byteOrder(), doc->maxAlign());
        bina::parse::ParseResult result;
        QVERIFY(parser.parse(doc->fields()->root(), readFile(sample("elements.bin")), result));

        const ParsedNode *version = find(result.records.front(), "version");
        QCOMPARE(doc->formats()->format("version_text", version->target, version->value), QStringLiteral("v1.2.300"));

        const ParsedNode *min = find(result.records.front(), "header.bounds.min");
        QCOMPARE(doc->formats()->format("point", min->target, min->value), QStringLiteral("(1, 2, 3)"));
    }

    void emptySchemaFails()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[]})");
        QVERIFY(doc);

        bina::parse::BinaryParser parser(doc->types(), doc->byteOrder(), doc->maxAlign());
        bina::parse::ParseResult result;

        const bina::schema::OperationResult parsed = parser.parse(doc->fields()->root(), QByteArray(4, 'x'), result);
        QVERIFY(!parsed);
        QVERIFY(!parsed.error().isEmpty());
    }

    void autoPadIsNotParsed()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::parse::BinaryParser parser(doc->types(), doc->byteOrder(), doc->maxAlign());
        bina::parse::ParseResult result;
        QVERIFY(parser.parse(doc->fields()->root(), readFile(sample("elements.bin")), result));

        // header tem um auto_pad na estrutura, mas nenhum nó correspondente no resultado.
        QVERIFY(!padSizes(findField(doc->fields()->root(), "header")).isEmpty());
        QCOMPARE(find(result.records.front(), "header")->children.size(), 3);
    }

    void recordsAreParsedInBatches()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[{"name":"v","type":"uint16"}]})");
        QVERIFY(doc);

        QByteArray data;
        for (quint16 i = 0; i < 2500; ++i)
            data.append(char(i & 0xFF)).append(char(i >> 8));

        bina::parse::BinaryParser parser(doc->types(), doc->byteOrder(), doc->maxAlign());
        bina::parse::ParseResult result;
        QVERIFY(parser.parse(doc->fields()->root(), data, result, 1000));

        QCOMPARE(result.recordCount(), qint64(2500));
        QCOMPARE(result.parsedCount(), qint64(1000));
        QVERIFY(!result.isComplete());

        // Os registros já lidos não mudam de endereço quando chegam outros.
        const ParsedNode *first = &result.records.front();
        QVERIFY(parser.parseMore(result, data, 1000));
        QCOMPARE(&result.records.front(), first);
        QCOMPARE(result.parsedCount(), qint64(2000));

        QVERIFY(parser.parseMore(result, data));                       // o resto
        QVERIFY(result.isComplete());
        QCOMPARE(result.records.back().instance, qint64(2499));
        QCOMPARE(result.records.back().children.first().value.toUnsigned(), quint64(2499));

        QVERIFY(!parser.parseMore(result, data.left(10)));             // outros dados: recusado
    }

    void invalidMaxAlignDoesNotCrash()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        bina::parse::BinaryParser parser(doc->types(), doc->byteOrder(), 0);
        bina::parse::ParseResult result;

        QVERIFY(parser.parse(doc->fields()->root(), readFile(sample("elements.bin")), result));
        QVERIFY(result.recordSize > 0);
    }
};

QTEST_GUILESS_MAIN(tst_Parser)
#include "tst_parser.moc"

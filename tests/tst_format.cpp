#include <limits>

#include <QtTest>

#include "bina/format/Formatter.h"
#include "bina/format/FormatRegistry.h"
#include "bina/format/PatternPreview.h"
#include "bina/format/PrintfFormat.h"
#include "bina/format/Value.h"
#include "bina/format/ValueDecoder.h"

using namespace bina::format;
using bina::schema::Representation;

namespace {

QByteArray bytes(std::initializer_list<int> values)
{
    QByteArray out;

    for (int v : values)
        out.append(char(v));

    return out;
}

}

// Decodificação dos bytes, formatadores embutidos e padrões printf.
class tst_Format : public QObject
{
    Q_OBJECT

private slots:

    void decodeIntegersWithByteOrder()
    {
        const QByteArray data = bytes({0x12, 0x34});

        QCOMPARE(decodeValue(Representation::Unsigned, 2, 1, data, QSysInfo::LittleEndian).toUnsigned(), quint64(0x3412));
        QCOMPARE(decodeValue(Representation::Unsigned, 2, 1, data, QSysInfo::BigEndian).toUnsigned(), quint64(0x1234));
    }

    void decodeSignedExtendsTheSign()
    {
        const Value v = decodeValue(Representation::Signed, 4, 1, bytes({0xFE, 0xFF, 0xFF, 0xFF}), QSysInfo::LittleEndian);

        QCOMPARE(v.type(), Value::Signed);
        QCOMPARE(v.toSigned(), qint64(-2));
        QCOMPARE(v.toRawBits(), quint64(0xFFFFFFFE));
    }

    void decodeFloatAndDouble()
    {
        const float f = 3.14f;
        const double d = 0.1;

        QCOMPARE(decodeValue(Representation::Float, 4, 1, QByteArray(reinterpret_cast<const char *>(&f), 4),
                             QSysInfo::LittleEndian).toDouble(), double(f));
        QCOMPARE(decodeValue(Representation::Float, 8, 1, QByteArray(reinterpret_cast<const char *>(&d), 8),
                             QSysInfo::LittleEndian).toDouble(), d);
    }

    void decodeCharArrayStopsAtNul()
    {
        const Value v = decodeValue(Representation::Char, 1, 8, QByteArray("ab\tc\0zz\0", 8), QSysInfo::LittleEndian);

        QCOMPARE(v.type(), Value::Text);
        QCOMPARE(v.text(), QStringLiteral("ab\tc"));
        QCOMPARE(v.raw().size(), 8);
    }

    void decodeNeedsEnoughBytes()
    {
        QVERIFY(!decodeValue(Representation::Unsigned, 4, 1, bytes({1, 2}), QSysInfo::LittleEndian).isValid());
    }

    void integerFormatters()
    {
        FormatRegistry registry;
        const Target target{Representation::Signed, 1, false};
        const Value v = decodeValue(Representation::Signed, 4, 1, bytes({0xFE, 0xFF, 0xFF, 0xFF}), QSysInfo::LittleEndian);

        QCOMPARE(registry.format("dec", target, v), QStringLiteral("-2"));
        QCOMPARE(registry.format("hex", target, v), QStringLiteral("0xFFFFFFFE"));
        QCOMPARE(registry.format("oct", target, v), QStringLiteral("037777777776"));
        QCOMPARE(registry.format("bin", target, v), QStringLiteral("0b11111111111111111111111111111110"));
    }

    void floatUsesShortestRepresentationOfTheType()
    {
        FormatRegistry registry;
        const float f = 3.14f;
        const Value v = decodeValue(Representation::Float, 4, 1, QByteArray(reinterpret_cast<const char *>(&f), 4),
                                    QSysInfo::LittleEndian);

        QCOMPARE(registry.format("float", {Representation::Float, 1, false}, v), QStringLiteral("3.14"));
    }

    void stringBytesAndArrays()
    {
        FormatRegistry registry;
        const Value text = decodeValue(Representation::Char, 1, 4, QByteArray("A\nB\0", 4), QSysInfo::LittleEndian);
        const Value array = decodeValue(Representation::Unsigned, 2, 3, bytes({1, 0, 2, 0, 0xFF, 0xFF}), QSysInfo::LittleEndian);

        QCOMPARE(registry.format("string", {Representation::Char, 4, false}, text), QStringLiteral("\"A\\nB\""));
        QCOMPARE(registry.format("bytes", {Representation::Char, 4, false}, text), QStringLiteral("41 0A 42 00"));
        QCOMPARE(registry.format("dec", {Representation::Unsigned, 3, false}, array), QStringLiteral("[1, 2, 65535]"));
        QCOMPARE(registry.format("hex", {Representation::Unsigned, 3, false}, array), QStringLiteral("[0x0001, 0x0002, 0xFFFF]"));
    }

    void hiddenFormatShowsNothingForAnyField()
    {
        FormatRegistry registry;
        const Value number = decodeValue(Representation::Unsigned, 2, 1, bytes({1, 0}), QSysInfo::LittleEndian);
        const Value array = decodeValue(Representation::Unsigned, 2, 3, bytes({1, 0, 2, 0, 0xFF, 0xFF}), QSysInfo::LittleEndian);

        QVERIFY(registry.contains("hidden"));
        QVERIFY(registry.isCompatible("hidden", {Representation::Unsigned, 1, false}));
        QVERIFY(registry.isCompatible("hidden", {Representation::Float, 3, false}));
        QVERIFY(registry.isCompatible("hidden", {Representation::None, 1, true}));   // grupos também

        QCOMPARE(registry.format("hidden", {Representation::Unsigned, 1, false}, number), QString());
        QCOMPARE(registry.format("hidden", {Representation::Unsigned, 3, false}, array), QString());
        QVERIFY(!registry.addPattern("hidden", "%d"));                              // o nome já é de um formato
    }

    void compatibilityAndDefaults()
    {
        FormatRegistry registry;

        QVERIFY(registry.isCompatible("hex", {Representation::Unsigned, 1, false}));
        QVERIFY(!registry.isCompatible("float", {Representation::Unsigned, 1, false}));
        QVERIFY(registry.isCompatible("float", {Representation::Float, 1, false}));
        QVERIFY(!registry.isCompatible("hex", {Representation::None, 1, true}));
        QVERIFY(!registry.isCompatible("desconhecido", {Representation::Unsigned, 1, false}));

        QCOMPARE(registry.defaultId({Representation::Signed, 1, false}), QStringLiteral("dec"));
        QCOMPARE(registry.defaultId({Representation::Float, 1, false}), QStringLiteral("float"));
        QCOMPARE(registry.defaultId({Representation::Char, 4, false}), QStringLiteral("string"));
        QCOMPARE(registry.defaultId({Representation::None, 1, true}), QString());
    }

    void printfFlagsWidthAndConversions()
    {
        FormatRegistry registry;
        const Target any{Representation::None, 1, true};
        const Value v = Value::makeStruct({"a", "b", "c"},
            {Value::makeSigned(-2, 4, Representation::Signed), Value::makeSigned(-2, 4, Representation::Signed),
             Value::makeSigned(-2, 4, Representation::Signed)});

        QCOMPARE(registry.format("printf:%#x %5d|%-5d|", any, v), QStringLiteral("0xfffffffe    -2|-2   |"));
        QCOMPARE(registry.format("printf:%08.3f", any, Value::makeFloat(3.14159, 8)), QStringLiteral("0003.142"));
        QCOMPARE(registry.format("printf:%b|%#010b|%c", any,
                                 Value::makeStruct({"a", "b", "c"}, {Value::makeUnsigned(5, 1, Representation::Unsigned),
                                                                     Value::makeUnsigned(5, 1, Representation::Unsigned),
                                                                     Value::makeUnsigned('Z', 1, Representation::Char)})),
                 QStringLiteral("101|0b00000101|Z"));
    }

    void printfFieldsAndOffsets()
    {
        FormatRegistry registry;
        QVERIFY(registry.addPattern("data", "%(ano+1900)04d-%(mes+1)02d-%(dia)02d"));

        const Value v = Value::makeStruct({"ano", "mes", "dia"},
            {Value::makeSigned(126, 4, Representation::Signed), Value::makeSigned(9, 4, Representation::Signed),
             Value::makeSigned(21, 4, Representation::Signed)});

        QCOMPARE(registry.format("data", {Representation::None, 1, true}, v), QStringLiteral("2026-10-21"));
    }

    void printfAcceptsUnicodeFieldNames()
    {
        FormatRegistry registry;
        const Value v = Value::makeStruct({"média"}, {Value::makeSigned(7, 4, Representation::Signed)});

        QVERIFY(registry.addPattern("pt_br", "%(média)d"));
        QCOMPARE(registry.format("pt_br", {Representation::None, 1, true}, v), QStringLiteral("7"));
    }

    void printfMissingArgumentAndArrayOfStructs()
    {
        FormatRegistry registry;
        const Value one = Value::makeSigned(1, 4, Representation::Signed);

        QCOMPARE(registry.format("printf:%d %d", {Representation::Signed, 1, false}, one), QStringLiteral("1 <?>"));

        const Value item = Value::makeStruct({"h"}, {Value::makeSigned(14, 4, Representation::Signed)});
        QCOMPARE(registry.format("printf:%(h)02d", {Representation::None, 2, true}, Value::makeArray({item, item})),
                 QStringLiteral("[14, 14]"));
    }

    void invalidPatternsAreRejected()
    {
        FormatRegistry registry;

        QVERIFY(!PrintfFormat::compile("%(campo").isValid());
        QVERIFY(!PrintfFormat::compile("%q").isValid());
        QVERIFY(!PrintfFormat::compile("%").isValid());
        QVERIFY(!PrintfFormat::compile("%999999d").isValid());

        QVERIFY(!registry.addPattern("1invalido", "%d"));
        QVERIFY(!registry.addPattern("dec", "%d"));                  // já existe
        QVERIFY(!registry.addPattern("ruim", "%q"));
        QVERIFY(registry.find("printf:%(") == nullptr);
    }

    void patternTestWithNamedFields()
    {
        const PatternPreviewResult r = previewPattern("v%(major)d.%(minor)d.%(patch)d", "major=1, minor=2, patch=3");
        QVERIFY(r.ok);
        QCOMPARE(r.text, QStringLiteral("v1.2.3"));

        // Caminho com ponto, ajuste e valores reais.
        QCOMPARE(previewPattern("%(b.min)g/%(ano+1900)d", "b.min=2.5\nano=100").text, QStringLiteral("2.5/2000"));
    }

    void patternTestWithPositionalValues()
    {
        QCOMPARE(previewPattern("%d-%x-%s", "10, 255, abc").text, QStringLiteral("10-ff-abc"));
        QCOMPARE(previewPattern("%d", "-5").text, QStringLiteral("-5"));
        QCOMPARE(previewPattern("%x", "0x1F").text, QStringLiteral("1f"));
        QCOMPARE(previewPattern("%d %d", "1").text, QStringLiteral("1 <?>"));         // falta um valor
    }

    void patternTestReportsErrors()
    {
        const PatternPreviewResult badPattern = previewPattern("%q", "1");
        QVERIFY(!badPattern.ok);
        QVERIFY(!badPattern.error.isEmpty());

        const PatternPreviewResult badSample = previewPattern("%d", "1x2=3");
        QVERIFY(!badSample.ok);
        QVERIFY(badSample.error.contains("Nome de campo inválido"));
    }

    void suggestedSampleFitsThePattern()
    {
        QCOMPARE(suggestSample("(%(x)g, %(y)g)"), QStringLiteral("x=1.5, y=1.5"));
        QCOMPARE(suggestSample("%d %s %x"), QStringLiteral("1, texto, 2"));
        QCOMPARE(suggestSample("%(a)d %(a)d"), QStringLiteral("a=1"));
        QVERIFY(suggestSample("%q").isEmpty());

        // A sugestão sempre produz um resultado sem "<?>".
        for (const QString &spec : {QStringLiteral("v%(major)d.%(minor)d"), QStringLiteral("%d %f %s"), QStringLiteral("%(b.min)g")})
            QVERIFY(!previewPattern(spec, suggestSample(spec)).text.contains("<?>"));
    }

    void printfLargeUnsignedAndOffsetsKeepTheWidth()
    {
        FormatRegistry registry;
        const Target any{Representation::None, 1, true};
        const Value max = Value::makeUnsigned(std::numeric_limits<quint64>::max(), 8, Representation::Unsigned);
        const Value one = Value::makeUnsigned(1, 1, Representation::Unsigned);

        // Um uint64 acima de INT64_MAX não vira negativo em "%d".
        QCOMPARE(registry.format("printf:%d", any, max), QStringLiteral("18446744073709551615"));
        QCOMPARE(registry.format("printf:%d", any, Value::makeUnsigned(7, 8, Representation::Unsigned)),
                 QStringLiteral("7"));

        // O deslocamento é limitado à largura do tipo: 1 - 2 em um uint8 é 0xff.
        QCOMPARE(registry.format("printf:%(v-2)x", any, Value::makeStruct({"v"}, {one})), QStringLiteral("ff"));
        QCOMPARE(registry.format("printf:%(v-2)u", any, Value::makeStruct({"v"}, {one})), QStringLiteral("255"));
    }

    void textThatIsNotUtf8IsReadAsLatin1()
    {
        QCOMPARE(decodeText(QByteArray("ol\xc3\xa1")), QStringLiteral("olá"));          // UTF-8 válido
        QCOMPARE(decodeText(QByteArray("ol\xe1")), QString::fromLatin1("ol\xe1"));      // não é UTF-8

        const Value text = decodeValue(Representation::Char, 1, 4, QByteArray("\xe9t\xe9\0", 4), QSysInfo::LittleEndian);
        QCOMPARE(text.text(), QString::fromLatin1("\xe9t\xe9"));
        QVERIFY(!text.text().contains(QChar::ReplacementCharacter));
    }

    void lookupsDoNotChangeTheRegistry()
    {
        FormatRegistry registry;

        QVERIFY(!registry.find("printf:%("));
        QVERIFY(!registry.check("printf:%(").ok());
        QVERIFY(registry.check("printf:%(").error().contains("printf:%("));
        QVERIFY(registry.check("nope").error().contains("nope"));
        QVERIFY(registry.check("dec").ok());

        // Muitos padrões inline (como os digitados em um preview) não acumulam: o cache é limitado,
        // e um padrão que saiu dele volta a ser compilado.
        for (int i = 0; i < 1000; ++i)
            QVERIFY(registry.find(QStringLiteral("printf:%d-%1").arg(i)));

        QVERIFY(registry.find("printf:%d-0"));
    }

    void patternResultsCarryTheReason()
    {
        FormatRegistry registry;

        const bina::schema::OperationResult added = registry.addPattern("p", "%d");
        QVERIFY(added.ok());

        const bina::schema::OperationResult repeated = registry.addPattern("p", "%d");
        QVERIFY(!repeated.ok());
        QVERIFY(repeated.error().contains("já existe"));

        QVERIFY(!registry.setPattern("dec", "%x").ok());               // embutido não muda
        QVERIFY(registry.setPattern("p", "%x").ok());
    }
};

QTEST_GUILESS_MAIN(tst_Format)
#include "tst_format.moc"

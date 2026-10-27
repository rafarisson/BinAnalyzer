#include "TestSupport.h"

#include "bina/document/PaddingSynchronizer.h"
#include "bina/schema/TypeRegistry.h"

using namespace TestSupport;

// Layout dos campos, tamanho das structs e auto_pad.
class tst_Layout : public QObject
{
    Q_OBJECT

private slots:

    void padsBetweenFieldsAndAtTheEnd()
    {
        auto doc = documentFromJson(R"({"version":1,"maxAlign":4,"elements":[
            {"name":"a","type":"uint8"},{"name":"b","type":"uint32"},{"name":"c","type":"uint8"}]})");
        QVERIFY(doc);

        bina::schema::Field *root = doc->fields()->root();

        QCOMPARE(childNames(root, true), QStringList({"a", "auto_pad", "b", "c", "auto_pad"}));
        QCOMPARE(padSizes(root), QList<int>({3, 3}));
        QCOMPARE(doc->fields()->sizeInBytes(root), 12);
    }

    void noPadsWhenAligned()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[
            {"name":"a","type":"uint32"},{"name":"b","type":"uint16"},{"name":"c","type":"uint16"}]})");
        QVERIFY(doc);

        QCOMPARE(padSizes(doc->fields()->root()), QList<int>());
        QCOMPARE(doc->fields()->sizeInBytes(doc->fields()->root()), 8);
    }

    void maxAlignLimitsFieldAlignment()
    {
        auto doc = documentFromJson(R"({"version":1,"maxAlign":2,"elements":[
            {"name":"a","type":"uint8"},{"name":"b","type":"uint32"},{"name":"c","type":"uint8"}]})");
        QVERIFY(doc);

        QCOMPARE(padSizes(doc->fields()->root()), QList<int>({1, 1}));
        QCOMPARE(doc->fields()->sizeInBytes(doc->fields()->root()), 8);
    }

    void doubleAlignsTo8WhenAllowed()
    {
        auto doc = documentFromJson(R"({"version":1,"maxAlign":8,"elements":[
            {"name":"a","type":"uint8"},{"name":"d","type":"double"}]})");
        QVERIFY(doc);

        QCOMPARE(padSizes(doc->fields()->root()), QList<int>({7}));
        QCOMPARE(doc->fields()->sizeInBytes(doc->fields()->root()), 16);
    }

    void nestedGroupsAndCounts()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[
            {"name":"g","count":3,"elements":[{"name":"x","type":"uint8"},{"name":"y","type":"uint16"}]}]})");
        QVERIFY(doc);

        bina::schema::Field *group = findField(doc->fields()->root(), "g");
        QVERIFY(group);

        QCOMPARE(group->typeId(), QStringLiteral("struct"));
        QCOMPARE(padSizes(group), QList<int>({1}));                 // x(1) + pad(1) + y(2)
        QCOMPARE(doc->fields()->sizeInBytes(group), 3 * 4);       // 3 instâncias de 4 bytes
    }

    void customTypesUseTheirLayout()
    {
        auto doc = sampleDocument();
        QVERIFY(doc);

        const bina::schema::DataType *vec3 = doc->types()->find("vec3");
        const bina::schema::DataType *bone = doc->types()->find("bone");
        QVERIFY(vec3 && bone);

        // O tamanho de um struct vem da definição e do alinhamento máximo do documento.
        const bina::schema::LayoutCalculator calculator(doc->types(), doc->maxAlign());
        QCOMPARE(calculator.structLayout(vec3->definition()).size, 12);
        QCOMPARE(calculator.structLayout(bone->definition()).size, 44);
        QCOMPARE(doc->fields()->sizeInBytes(doc->fields()->root()), 196);
    }

    void registryStartsEmptyAndBuiltinsAreOptIn()
    {
        bina::schema::TypeRegistry empty;
        QVERIFY(empty.types().isEmpty());
        QVERIFY(!empty.contains("uint8"));

        auto builtins = bina::schema::TypeRegistry::withBuiltins();
        QVERIFY(builtins->contains("uint8"));
        QVERIFY(builtins->contains("struct"));
        QCOMPARE(builtins->find("uint32")->size(), 4);
        QCOMPARE(builtins->find("uint32")->category(), bina::schema::DataType::Primitive);
        QCOMPARE(builtins->find("struct")->category(), bina::schema::DataType::Struct);
    }

    void registryRefusalsCarryTheirReason()
    {
        auto registry = bina::schema::TypeRegistry::withBuiltins();

        const bina::schema::OperationResult duplicate = registry->add("uint8", 1, 1);
        QVERIFY(!duplicate);
        QVERIFY(duplicate.error().contains("já existe"));

        QVERIFY(!registry->add("x", 4, 3));                      // alinhamento que não é potência de 2
        QVERIFY(!registry->add("y", std::make_unique<bina::schema::Field>()));   // struct sem campos
        QVERIFY(registry->add("z", 2, 2));
        QVERIFY(bina::schema::Alignment::validateMax(8));
        QVERIFY(!bina::schema::Alignment::validateMax(6));
    }

    void changingMaxAlignResyncsPads()
    {
        auto doc = documentFromJson(R"({"version":1,"maxAlign":4,"elements":[
            {"name":"a","type":"uint8"},{"name":"b","type":"uint32"}]})");
        QVERIFY(doc);

        bina::schema::Field *root = doc->fields()->root();
        QCOMPARE(doc->fields()->sizeInBytes(root), 8);

        QVERIFY(doc->setMaxAlign(2));
        QCOMPARE(doc->maxAlign(), 2);
        QCOMPARE(padSizes(root), QList<int>({1}));
        QCOMPARE(doc->fields()->sizeInBytes(root), 6);

        QVERIFY(!doc->setMaxAlign(3));                              // não é potência de 2
        QCOMPARE(doc->maxAlign(), 2);
    }

    void unknownTypeIsRejected()
    {
        QString error;
        auto doc = documentFromJson(R"({"version":1,"elements":[{"name":"a","type":"nao_existe"}]})", &error);

        QVERIFY(!doc);
        QVERIFY(!error.isEmpty());
    }

    void structCannotContainItself()
    {
        QString error;
        auto doc = documentFromJson(R"({"version":1,"types":[
            {"id":"loop","elements":[{"name":"x","type":"loop"}]}],"elements":[{"name":"a","type":"loop"}]})", &error);

        QVERIFY(!doc);
        QVERIFY(error.contains("tipo desconhecido"));
    }

    void editingUpdatesPadsAndSizes()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[
            {"name":"a","type":"uint8"},{"name":"b","type":"uint16"}]})");
        QVERIFY(doc);

        bina::document::FieldTree *tree = doc->fields();
        bina::schema::Field *root = tree->root();

        QCOMPARE(padSizes(root), QList<int>({1}));

        QVERIFY(tree->setTypeId(findField(root, "b"), "uint8"));
        QCOMPARE(padSizes(root), QList<int>());                      // a(1) b(1): alinhado
        QCOMPARE(tree->sizeInBytes(root), 2);

        QVERIFY(tree->setTypeId(findField(root, "b"), "uint32"));
        QCOMPARE(padSizes(root), QList<int>({3}));                   // a(1) + pad(3) + b(4)
        QCOMPARE(tree->sizeInBytes(root), 8);

        QVERIFY(tree->setCount(findField(root, "a"), 3));
        QCOMPARE(padSizes(root), QList<int>({1}));                   // a(3) + pad(1) + b(4)
        QCOMPARE(tree->sizeInBytes(root), 8);
    }

    void invalidMaxAlignIsTreatedAsOne()
    {
        auto doc = documentFromJson(R"({"version":1,"elements":[
            {"name":"a","type":"uint8"},{"name":"b","type":"uint32"}]})");
        QVERIFY(doc);

        // Sem a proteção, 0 ou um valor negativo dividiriam por zero no cálculo do padding.
        for (const int maxAlign : {0, -4, 3}) {
            const bina::schema::LayoutCalculator calculator(doc->types(), maxAlign);

            QCOMPARE(calculator.maxAlign(), 1);
            QCOMPARE(calculator.structLayout(doc->fields()->root()).size, 5);   // sem padding
        }
    }

    void fieldWithChildrenIsAStructWhateverItsTypeId()
    {
        auto registry = bina::schema::TypeRegistry::withBuiltins();
        bina::schema::Field root;
        bina::schema::Field *group = root.insertChild(0, "group");

        QCOMPARE(group->typeId(), QStringLiteral("uint32"));
        QVERIFY(!group->isStruct());

        group->insertChild(0, "x")->setTypeId("uint8");
        group->insertChild(1, "y")->setTypeId("uint8");

        // O typeId guardado não mudou, mas o layout usa os filhos (2 bytes, não os 4 de um uint32).
        QVERIFY(group->isStruct());
        QCOMPARE(bina::schema::LayoutCalculator(registry.get(), 4).sizeOf(group), 2);
    }

    void fieldSettersTellRejectionFromNoChange()
    {
        using Edit = bina::schema::Field::Edit;
        bina::schema::Field field("a");

        QCOMPARE(field.setName("a"), Edit::Unchanged);
        QCOMPARE(field.setName("  "), Edit::Invalid);
        QCOMPARE(field.setName("b"), Edit::Changed);
        QCOMPARE(field.setCount(0), Edit::Invalid);
        QCOMPARE(field.setCount(1), Edit::Unchanged);
        QCOMPARE(field.setTypeId(""), Edit::Invalid);
    }

    void fieldIdsAreStableAndNeverReused()
    {
        bina::schema::Field root;
        bina::schema::Field *a = root.insertChild(0, "a");
        const bina::schema::Field::Id id = a->id();

        a->setName("renamed");
        root.insertChild(0, "before");                               // a muda de linha
        QCOMPARE(a->id(), id);

        const auto copy = a->clone();
        QVERIFY(copy->id() != id);

        root.removeChild(a->row());
        QVERIFY(root.insertChild(0, "new")->id() != id);
    }

    void dependsOnFollowsSharedTypesOnce()
    {
        // Um tipo usado por muitos campos, em vários níveis: a consulta continua correta.
        auto doc = documentFromJson(R"({"version":1,"types":[
            {"id":"leaf","elements":[{"name":"v","type":"uint8"}]},
            {"id":"mid","elements":[{"name":"a","type":"leaf"},{"name":"b","type":"leaf"},{"name":"c","type":"leaf"}]},
            {"id":"top","elements":[{"name":"a","type":"mid"},{"name":"b","type":"mid"},{"name":"c","type":"mid"}]}],
            "elements":[{"name":"t","type":"top"}]})");
        QVERIFY(doc);

        QVERIFY(doc->types()->dependsOn("top", "leaf"));
        QVERIFY(doc->types()->dependsOn("mid", "leaf"));
        QVERIFY(!doc->types()->dependsOn("leaf", "top"));
        QVERIFY(!doc->types()->dependsOn("top", "uint16"));
    }

    void paddingPlanIsComputedWithoutChangingTheTree()
    {
        using Step = bina::document::PaddingSynchronizer::Step;

        auto registry = bina::schema::TypeRegistry::withBuiltins();
        bina::schema::Field root;
        root.insertChild(0, "a")->setTypeId("uint8");
        root.insertChild(1, "b")->setTypeId("uint32");

        const bina::document::PaddingSynchronizer synchronizer(registry.get(), 4);

        // a(1) + pad(3) + b(4): um auto_pad novo antes de b.
        QCOMPARE(synchronizer.plan(&root), QList<Step>({{Step::Insert, 1, 3}}));
        QCOMPARE(root.childCount(), 2);

        // Um auto_pad do tamanho errado é reaproveitado; um que sobra é removido.
        root.insertExistingChild(1, bina::schema::Field::makeAutoPad(1));
        root.insertExistingChild(3, bina::schema::Field::makeAutoPad(2));
        QCOMPARE(synchronizer.plan(&root), QList<Step>({{Step::Resize, 1, 3}, {Step::Remove, 3}}));

        // Sem campos, todos os filhos saem.
        bina::schema::Field onlyPads;
        onlyPads.insertExistingChild(0, bina::schema::Field::makeAutoPad(2));
        QCOMPARE(synchronizer.plan(&onlyPads), QList<Step>({{Step::Remove, 0}}));
    }
};

QTEST_GUILESS_MAIN(tst_Layout)
#include "tst_layout.moc"

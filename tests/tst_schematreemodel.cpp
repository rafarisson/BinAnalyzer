#include "TestSupport.h"

#include "bina/model/DropTargetResolver.h"
#include "bina/model/FieldMimeCodec.h"
#include "bina/model/EditorRoles.h"
#include "bina/model/PendingMoveGuard.h"
#include "bina/model/SchemaColumns.h"
#include "bina/model/SchemaTreeModel.h"
#include "bina/model/FieldRef.h"

using namespace TestSupport;
using bina::model::SchemaTreeModel;
using bina::schema::Field;

namespace {

// O documento de exemplo com o model e o verificador de contrato do Qt acompanhando tudo.
struct Fixture
{
    std::unique_ptr<bina::document::Document> doc{sampleDocument()};
    SchemaTreeModel model{doc->fields()};
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::Fatal};

    // A raiz não aparece: seus filhos são os itens de topo.
    QModelIndex root() const { return {}; }

    bool isPad(const QModelIndex &i) const { return model.data(i, SchemaTreeModel::IsAutoPadRole).toBool(); }

    QModelIndex find(const QString &name, const QModelIndex &parent = {}) const
    {
        for (int r = 0; r < model.rowCount(parent); ++r) {
            const QModelIndex i = model.index(r, 0, parent);

            if (!isPad(i) && model.data(i, SchemaTreeModel::NameRole).toString() == name)
                return i;

            if (const QModelIndex c = find(name, i); c.isValid())
                return c;
        }

        return {};
    }

    QModelIndex firstPad(const QModelIndex &parent) const
    {
        for (int r = 0; r < model.rowCount(parent); ++r)
            if (isPad(model.index(r, 0, parent)))
                return model.index(r, 0, parent);

        return {};
    }

    QStringList children(const QModelIndex &parent) const
    {
        QStringList names;

        for (int r = 0; r < model.rowCount(parent); ++r) {
            const QModelIndex i = model.index(r, 0, parent);

            if (!isPad(i))
                names << model.data(i, SchemaTreeModel::NameRole).toString();
        }

        return names;
    }

    // Todas as colunas da linha, como a view entrega ao arrastar.
    QModelIndexList row(const QModelIndex &index) const
    {
        QModelIndexList list;

        for (int c = 0; c < model.columnCount(); ++c)
            list << model.index(index.row(), c, index.parent());

        return list;
    }

    QMimeData *mimeFor(const QString &name) const { return model.mimeData(row(find(name))); }

    static Field *field(const QModelIndex &i) { return static_cast<Field *>(i.internalPointer()); }
};

}

// O model da árvore de campos: flags, edição, menu e o protocolo de arrastar e soltar.
class tst_SchemaTreeModel : public QObject
{
    Q_OBJECT

private slots:

    void flags()
    {
        Fixture f;
        auto flagsOf = [&](const QModelIndex &i, int column) { return f.model.flags(f.model.index(i.row(), column, i.parent())); };

        const QModelIndex flags = f.find("flags");
        QVERIFY(flagsOf(flags, 0) & Qt::ItemIsDragEnabled);
        QVERIFY(flagsOf(flags, 0) & Qt::ItemIsDropEnabled);
        QVERIFY(!(flagsOf(flags, 1) & Qt::ItemIsDragEnabled));      // só o nome arrasta e recebe
        QVERIFY(!(flagsOf(flags, 1) & Qt::ItemIsDropEnabled));
        QVERIFY(flagsOf(flags, 1) & Qt::ItemIsEditable);
        QVERIFY(!(flagsOf(flags, SchemaTreeModel::SizeInBytesColumn) & Qt::ItemIsEditable));

        const QModelIndex pad = f.firstPad(f.find("header"));
        QVERIFY(pad.isValid());
        QVERIFY(!(flagsOf(pad, 0) & (Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable)));

        QVERIFY(f.model.flags(f.root()) & Qt::ItemIsDropEnabled);
        QCOMPARE(f.model.supportedDropActions(), Qt::MoveAction);

        // O tipo de um grupo não é editável.
        QVERIFY(!(flagsOf(f.find("header"), SchemaTreeModel::TypeIdColumn) & Qt::ItemIsEditable));
    }

    void editorRolesTellTheViewWhichEditorToOpen()
    {
        Fixture f;
        const QModelIndex magic = f.find("magic");
        auto at = [&](int column) { return f.model.index(magic.row(), column, magic.parent()); };

        QCOMPARE(f.model.data(at(SchemaTreeModel::NameColumn), bina::model::EditorKindRole).toString(), QStringLiteral("text"));
        QCOMPARE(f.model.data(at(SchemaTreeModel::TypeIdColumn), bina::model::EditorKindRole).toString(), QStringLiteral("combo"));
        QCOMPARE(f.model.data(at(SchemaTreeModel::CountColumn), bina::model::EditorKindRole).toString(), QStringLiteral("spin"));
        QCOMPARE(f.model.data(at(SchemaTreeModel::FormatIdColumn), bina::model::EditorKindRole).toString(), QStringLiteral("combo"));
        QVERIFY(f.model.data(at(SchemaTreeModel::SizeInBytesColumn), bina::model::EditorKindRole).toString().isEmpty());
        QVERIFY(f.model.data(f.find("header").siblingAtColumn(SchemaTreeModel::TypeIdColumn),
                             bina::model::EditorKindRole).toString().isEmpty());

        QCOMPARE(f.model.data(at(SchemaTreeModel::CountColumn), bina::model::MinimumRole).toInt(), 1);
        QVERIFY(f.model.data(at(SchemaTreeModel::CountColumn), bina::model::MaximumRole).toInt() > 1);

        // Tipos: os primitivos e os da estrutura, sem o marcador "struct".
        QStringList types;
        for (const QVariant &c : f.model.data(at(SchemaTreeModel::TypeIdColumn), bina::model::ChoicesRole).toList())
            types << c.toMap().value("value").toString();

        QVERIFY(types.contains("uint32"));
        QVERIFY(types.contains("vec3"));
        QVERIFY(!types.contains("struct"));

        // Formatos: a primeira opção é "(padrão)" com valor vazio.
        const QVariantList formats = f.model.data(at(SchemaTreeModel::FormatIdColumn), bina::model::ChoicesRole).toList();
        QVERIFY(formats.size() > 1);
        QCOMPARE(formats.first().toMap().value("value").toString(), QString());
        QCOMPARE(formats.first().toMap().value("text").toString(), QStringLiteral("(padrão)"));
    }

    void editingThroughSetData()
    {
        Fixture f;
        const QModelIndex magic = f.find("magic");
        auto at = [&](int column) { return f.model.index(magic.row(), column, magic.parent()); };

        QVERIFY(f.model.setData(at(SchemaTreeModel::CountColumn), 8));
        QCOMPARE(f.model.data(at(SchemaTreeModel::SizeInBytesColumn), Qt::DisplayRole).toInt(), 8);
        QCOMPARE(f.doc->fields()->sizeInBytes(f.doc->fields()->root()), 200);

        QVERIFY(f.model.setData(at(SchemaTreeModel::TypeIdColumn), "uint16"));
        QVERIFY(f.model.setData(at(SchemaTreeModel::NameColumn), "assinatura"));
        QVERIFY(!f.model.setData(at(SchemaTreeModel::SizeInBytesColumn), 1));
    }

    void rejectedEditsAreForwardedToTheView()
    {
        Fixture f;
        QSignalSpy errors(&f.model, &SchemaTreeModel::errorOccurred);
        const QModelIndex magic = f.find("magic");

        QVERIFY(!f.model.setData(magic, "version"));
        QCOMPARE(errors.size(), 1);
        QVERIFY(!errors.first().first().toString().isEmpty());
    }

    void dragAndDropBetweenSiblingsAndTheViewsRemoveRows()
    {
        Fixture f;
        const QModelIndex checksum = f.find("checksum");
        QPersistentModelIndex persistent(checksum);        // a view guarda a seleção assim
        std::unique_ptr<QMimeData> mime(f.model.mimeData(f.row(checksum)));
        QVERIFY(mime);

        const int rowOfVersion = f.find("version").row();
        QVERIFY(f.model.canDropMimeData(mime.get(), Qt::MoveAction, rowOfVersion, 0, f.root()));
        QVERIFY(!f.model.canDropMimeData(mime.get(), Qt::CopyAction, rowOfVersion, 0, f.root()));

        Field *before = Fixture::field(checksum);
        QSignalSpy revealed(&f.model, &SchemaTreeModel::revealRequested);

        QVERIFY(f.model.dropMimeData(mime.get(), Qt::MoveAction, rowOfVersion, 0, f.root()));
        QCOMPARE(revealed.size(), 1);
        QCOMPARE(f.children(f.root()).mid(0, 3), QStringList({"magic", "checksum", "version"}));
        QCOMPARE(Fixture::field(f.find("checksum")), before);        // a identidade é preservada

        // Depois de um MoveAction a view pede removeRows() da origem: já foi movida, é ignorado.
        QVERIFY(f.model.removeRows(persistent.row(), 1, persistent.parent()));
        QVERIFY(f.find("checksum").isValid());

        // A espera vale uma vez: a remoção seguinte é real.
        const QModelIndex again = f.find("checksum");
        QVERIFY(f.model.removeRows(again.row(), 1, again.parent()));
        QVERIFY(!f.find("checksum").isValid());
    }

    void droppingOntoACommonFieldCreatesAGroupKeepingTheOldField()
    {
        Fixture f;
        const QModelIndex magic = f.find("magic");
        Field *magicField = Fixture::field(magic);
        Field *checksumField = Fixture::field(f.find("checksum"));

        std::unique_ptr<QMimeData> mime(f.mimeFor("checksum"));
        QVERIFY(f.model.canDropMimeData(mime.get(), Qt::MoveAction, -1, -1, magic));
        QVERIFY(f.model.dropMimeData(mime.get(), Qt::MoveAction, -1, -1, magic));

        const QModelIndex group = f.find("group_1");
        QVERIFY(group.isValid());
        QCOMPARE(f.children(group), QStringList({"magic", "checksum"}));
        QCOMPARE(f.children(f.root()), QStringList({"group_1", "version", "header", "bones"}));
        QCOMPARE(Fixture::field(f.find("magic")), magicField);          // o campo antigo foi mantido
        QCOMPARE(Fixture::field(f.find("checksum")), checksumField);
        QCOMPARE(magicField->count(), 4);
        QCOMPARE(f.doc->fields()->sizeInBytes(Fixture::field(group)), 8);
    }

    void droppingOntoAnExistingStructAppendsInsideIt()
    {
        Fixture f;
        std::unique_ptr<QMimeData> mime(f.mimeFor("checksum"));

        QVERIFY(f.model.dropMimeData(mime.get(), Qt::MoveAction, -1, -1, f.find("header")));
        QCOMPARE(f.children(f.find("header")).last(), QStringLiteral("checksum"));
        QVERIFY(!f.find("group_1").isValid());
    }

    void droppingOnAPadRowMeansTheNextField()
    {
        Fixture f;
        const QModelIndex pad = f.firstPad(f.find("header"));
        QVERIFY(pad.isValid());

        std::unique_ptr<QMimeData> mime(f.mimeFor("checksum"));
        QVERIFY(f.model.dropMimeData(mime.get(), Qt::MoveAction, pad.row(), 0, f.find("header")));

        QCOMPARE(f.children(f.find("header")).mid(0, 3), QStringList({"flags", "checksum", "bone_count"}));
    }

    void refusedDrops()
    {
        Fixture f;
        const QModelIndex checksum = f.find("checksum");
        std::unique_ptr<QMimeData> mime(f.mimeFor("checksum"));

        QVERIFY(!f.model.canDropMimeData(mime.get(), Qt::MoveAction, -1, -1, checksum));      // sobre si mesmo

        std::unique_ptr<QMimeData> header(f.mimeFor("header"));
        QVERIFY(!f.model.canDropMimeData(header.get(), Qt::MoveAction, -1, -1, f.find("flags")));  // dentro de si
        QVERIFY(!f.model.dropMimeData(header.get(), Qt::MoveAction, -1, -1, f.find("flags")));

        // Sem mudar de posição.
        std::unique_ptr<QMimeData> magic(f.mimeFor("magic"));
        const int rowOfMagic = f.find("magic").row();
        QVERIFY(!f.model.canDropMimeData(magic.get(), Qt::MoveAction, rowOfMagic, 0, f.root()));
        QVERIFY(!f.model.canDropMimeData(magic.get(), Qt::MoveAction, rowOfMagic + 1, 0, f.root()));
    }

    void onlyOneFieldCanBeDragged()
    {
        Fixture f;

        const QModelIndexList two = f.row(f.find("checksum")) + f.row(f.find("magic"));
        QVERIFY(f.model.mimeData(two) == nullptr);
        QVERIFY(f.model.mimeData({f.firstPad(f.find("header"))}) == nullptr);   // auto_pad não arrasta
        QVERIFY(f.model.mimeData({f.root()}) == nullptr);                       // nem a raiz
    }

    void mimeDataIsBoundToTheModelAndTheCurrentStructure()
    {
        Fixture a, b;
        std::unique_ptr<QMimeData> mime(a.mimeFor("checksum"));

        QVERIFY(a.model.canDropMimeData(mime.get(), Qt::MoveAction, 1, 0, a.root()));
        QVERIFY(!b.model.canDropMimeData(mime.get(), Qt::MoveAction, 1, 0, b.root()));   // outro model

        a.model.insertRows(0, 1, a.root());                                              // a estrutura mudou
        QVERIFY(!a.model.canDropMimeData(mime.get(), Qt::MoveAction, 2, 0, a.root()));
        QVERIFY(!a.model.dropMimeData(mime.get(), Qt::MoveAction, 2, 0, a.root()));
    }

    void editingTheDraggedFieldKeepsTheDragValid()
    {
        // O campo arrastado é conferido pelo id, não pelo nome: renomeá-lo (ou mudar o count) no
        // meio do arrasto não perde a referência.
        Fixture f;
        std::unique_ptr<QMimeData> mime(f.mimeFor("checksum"));
        Field *checksum = Fixture::field(f.find("checksum"));

        QVERIFY(f.doc->fields()->setName(checksum, "crc"));

        const int rowOfVersion = f.find("version").row();
        QVERIFY(f.model.canDropMimeData(mime.get(), Qt::MoveAction, rowOfVersion, 0, f.root()));
        QVERIFY(f.model.dropMimeData(mime.get(), Qt::MoveAction, rowOfVersion, 0, f.root()));
        QCOMPARE(Fixture::field(f.find("crc")), checksum);
    }

    void invalidMimePayloadsAreRefused()
    {
        Fixture f;
        std::unique_ptr<QMimeData> mime(f.mimeFor("checksum"));
        const QString type = bina::model::FieldMimeCodec::mimeTypes().first();
        const QByteArray good = mime->data(type);

        auto decodes = [&](const QByteArray &bytes) {
            QMimeData m;
            m.setData(type, bytes);
            bina::model::FieldRef ref;
            return bina::model::FieldMimeCodec::decode(&m, ref);
        };

        QVERIFY(decodes(good));
        QVERIFY(!decodes({}));
        QVERIFY(!decodes(good.left(good.size() - 1)));                   // truncado
        QVERIFY(!decodes(good + QByteArray(1, 'x')));                    // bytes sobrando
        QVERIFY(!decodes(QByteArray(good).replace(0, 1, "Z")));          // magic errado
        QVERIFY(!decodes(QByteArray(5000, 'a')));                        // grande demais

        QByteArray version = good;
        version[4] = 0x7f;                                               // versão desconhecida
        QVERIFY(!decodes(version));

        QByteArray deep;
        {
            QDataStream s(&deep, QIODevice::WriteOnly);
            s.setVersion(QDataStream::Qt_6_0);
            s << quint32(0x42494E41) << quint16(1) << quint64(7) << quint32(1) << QString("x|y|1") << quint16(500);
            for (int i = 0; i < 500; ++i)
                s << qint32(0);
        }
        QVERIFY(!decodes(deep));                                         // profundidade excessiva
    }

    void menuActionsAreDecidedByTheModel()
    {
        Fixture f;

        QCOMPARE(f.model.actions(f.find("flags")).size(), 4);
        QVERIFY(f.model.actions(f.firstPad(f.find("header"))).isEmpty());

        // Filho da raiz: sem "abaixo do pai".
        QVERIFY(!f.model.actions(f.find("magic")).at(1).toMap().value("enabled").toBool());
        QVERIFY(f.model.actions(f.find("flags")).at(1).toMap().value("enabled").toBool());
    }

    void triggeringActions()
    {
        Fixture f;
        QSignalSpy revealed(&f.model, &SchemaTreeModel::revealRequested);

        QVERIFY(f.model.trigger(f.find("flags"), "insertBelow"));
        QCOMPARE(revealed.size(), 1);

        const QModelIndex created = revealed.first().first().value<QModelIndex>();
        QCOMPARE(f.model.data(created, SchemaTreeModel::NameRole).toString(), QStringLiteral("field_1"));

        QVERIFY(f.model.trigger(created, "remove"));
        QVERIFY(!f.find("field_1").isValid());

        QVERIFY(f.model.trigger(f.find("weight"), "insertBelowParent"));        // vira irmão de "extra"
        QVERIFY(f.children(f.find("bounds")).contains("field_1"));

        QVERIFY(f.model.trigger(f.find("flags"), "insertInside"));              // flags vira grupo
        QCOMPARE(f.model.data(f.find("flags"), SchemaTreeModel::TypeIdRole).toString(), QStringLiteral("struct"));

        QVERIFY(f.firstPad(f.find("extra")).isValid());
        QVERIFY(!f.model.trigger(f.firstPad(f.find("extra")), "remove"));
        QVERIFY(!f.model.trigger(f.root(), "remove"));
        QVERIFY(!f.model.trigger(f.find("magic"), "nao_existe"));
    }

    void dropByIndexHasNoPendingRemoval()
    {
        Fixture f;
        const QModelIndex checksum = f.find("checksum");

        QVERIFY(f.model.canDropAt(checksum, f.find("magic"), SchemaTreeModel::Onto));
        QVERIFY(f.model.dropAt(checksum, f.find("magic"), SchemaTreeModel::Onto));

        // Não houve MoveAction da view: uma remoção do campo logo depois é real.
        const QModelIndex moved = f.find("checksum");
        QVERIFY(f.model.removeRows(moved.row(), 1, moved.parent()));
        QVERIFY(!f.find("checksum").isValid());

        QVERIFY(!f.model.dropAt(f.firstPad(f.find("header")), f.find("magic"), SchemaTreeModel::Onto));
    }

    void standardRowOperations()
    {
        Fixture f;

        QVERIFY(f.model.moveRows(f.root(), f.find("checksum").row(), 1, f.root(), 1));
        QCOMPARE(f.children(f.root()).mid(0, 2), QStringList({"magic", "checksum"}));

        QVERIFY(f.model.insertRows(1, 1, f.root()));
        QCOMPARE(f.children(f.root()).at(1), QStringLiteral("field_1"));

        QVERIFY(!f.model.moveRows(f.root(), 0, 2, f.root(), 3));                // só uma linha
    }

    void sizesOfTheAncestorsFollowAnEdit()
    {
        Fixture f;
        auto sizeOf = [&](const QString &name) {
            return f.model.data(f.find(name).siblingAtColumn(SchemaTreeModel::SizeInBytesColumn), Qt::DisplayRole).toInt();
        };

        QCOMPARE(sizeOf("header"), 52);

        QVERIFY(f.model.setData(f.find("bone_count").siblingAtColumn(SchemaTreeModel::TypeIdColumn), "uint32"));
        QCOMPARE(sizeOf("header"), 56);
        QCOMPARE(f.doc->fields()->sizeInBytes(f.doc->fields()->root()), 200);
    }

    void headersAndAlignmentComeFromTheColumnTable()
    {
        Fixture f;

        QCOMPARE(f.model.headerData(SchemaTreeModel::NameColumn, Qt::Horizontal).toString(), QStringLiteral("Nome"));
        QVERIFY(!f.model.headerData(SchemaTreeModel::ColumnCount, Qt::Horizontal).isValid());
        QVERIFY(!bina::model::SchemaColumns::at(-1));
        QCOMPARE(bina::model::SchemaColumns::forRole(SchemaTreeModel::CountRole), bina::model::SchemaColumns::at(SchemaTreeModel::CountColumn));
        QVERIFY(!bina::model::SchemaColumns::forRole(SchemaTreeModel::IsAutoPadRole));

        const QModelIndex flags = f.find("flags");
        QCOMPARE(f.model.data(flags.siblingAtColumn(SchemaTreeModel::CountColumn), Qt::TextAlignmentRole), QVariant(Qt::AlignRight));
        QVERIFY(!f.model.data(flags, Qt::TextAlignmentRole).isValid());

        // O total é só leitura, por qualquer caminho.
        QVERIFY(!f.model.setData(flags.siblingAtColumn(SchemaTreeModel::SizeInBytesColumn), 4));
        QVERIFY(!f.model.setData(flags, 4, SchemaTreeModel::SizeInBytesRole));
    }

    void dropTargetsSkipPadsAndFallBackToTheParent()
    {
        using bina::model::resolveDropTarget;
        using bina::document::FieldTree;

        Fixture f;
        Field *header = Fixture::field(f.find("header"));
        const QModelIndex pad = f.firstPad(f.find("header"));
        QVERIFY(pad.isValid());

        QVERIFY(!resolveDropTarget(nullptr, 0));

        auto onto = resolveDropTarget(header, -1);
        QVERIFY(onto && onto->target == header && onto->zone == FieldTree::Onto);

        // Uma linha de pad vale o próximo campo.
        auto atPad = resolveDropTarget(header, pad.row());
        QVERIFY(atPad && !atPad->target->isAutoPad() && atPad->zone == FieldTree::Before);
        QVERIFY(atPad->target->row() > pad.row());

        // Depois do fim: após o último campo (não pad).
        auto atEnd = resolveDropTarget(header, header->childCount() + 5);
        QVERIFY(atEnd && !atEnd->target->isAutoPad() && atEnd->zone == FieldTree::After);
        QCOMPARE(atEnd->target->parent(), header);

        Field empty;
        auto inEmpty = resolveDropTarget(&empty, 0);
        QVERIFY(inEmpty && inEmpty->target == &empty && inEmpty->zone == FieldTree::Onto);
    }

    void pendingMoveIsRecognizedOnceAndExpires()
    {
        using namespace std::chrono_literals;

        Field a, b;

        bina::model::PendingMoveGuard guard;
        QVERIFY(!guard.take(&a));

        guard.arm(&a);
        QVERIFY(guard.take(&a));
        QVERIFY(!guard.take(&a));            // uma única vez

        guard.arm(&a);
        QVERIFY(!guard.take(&b));            // outra remoção encerra a espera
        QVERIFY(!guard.take(&a));

        guard.arm(&a);
        guard.reset();
        QVERIFY(!guard.take(&a));

        bina::model::PendingMoveGuard expired{0ms};
        expired.arm(&a);
        QVERIFY(!expired.take(&a));
    }
};

QTEST_GUILESS_MAIN(tst_SchemaTreeModel)
#include "tst_schematreemodel.moc"

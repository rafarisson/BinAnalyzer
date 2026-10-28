import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Tipos customizados do documento: o ComboBox escolhe o tipo e a árvore edita os campos dele, com as
// mesmas regras da estrutura. As edições ficam em um rascunho até "Aplicar"; "Restaurar" volta à
// definição do documento. Sem tipos customizados o ComboBox fica desabilitado e a árvore escondida.
// A segunda linha liga o tipo à biblioteca de tipos (salvar nela, atualizar a partir dela) e remove o
// tipo do documento, o que só é aceito se nenhum campo o usa.
ColumnLayout {
    id: root

    required property AppController app
    required property TypeDefinitionController typeController

    readonly property bool hasTypes: typeController.typeIds.length > 0
    readonly property string current: typeController.currentType

    // Gravar por cima de um tipo da biblioteca pede confirmação.
    function saveToLibrary() {
        if (app.libraryHasType(current))
            overwrite.open()
        else
            app.saveTypeToLibrary(current)
    }

    function expandAll() {
        tree.expandAll()
    }

    function collapseAll() {
        tree.collapseAll()
    }

    spacing: 0

    RowLayout {
        id: header

        Layout.fillWidth: true
        Layout.margins: Theme.pagePadding

        ComboBox {
            id: combo

            model: root.typeController.typeIds
            enabled: root.hasTypes
            Layout.fillWidth: true

            onActivated: root.typeController.currentType = currentText

            // O ComboBox troca o próprio currentIndex ao escolher; a ligação o mantém igual ao model.
            Binding {
                target: combo
                property: "currentIndex"
                value: root.typeController.typeIds.indexOf(root.typeController.currentType)
            }
        }

        Button {
            text: qsTr("Novo...")
            onClicked: newType.open()

            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: qsTr("Cria um tipo com um campo inicial, para editar em seguida.")
        }

        Button {
            text: qsTr("Aplicar")
            enabled: root.typeController.modified
            highlighted: root.typeController.modified
            onClicked: root.typeController.apply()

            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: qsTr("Grava a definição no tipo. Os dados precisam ser analisados novamente.")
        }

        Button {
            text: qsTr("Restaurar")
            enabled: root.typeController.modified
            onClicked: root.typeController.restore()
        }
    }

    RowLayout {
        visible: root.hasTypes
        Layout.fillWidth: true
        Layout.leftMargin: Theme.pagePadding
        Layout.rightMargin: Theme.pagePadding
        Layout.bottomMargin: Theme.pagePadding

        Label {
            text: {
                switch (root.typeController.libraryState) {
                case TypeDefinitionController.SameAsLibrary:
                    return qsTr("Igual ao da biblioteca")
                case TypeDefinitionController.DiffersFromLibrary:
                    return qsTr("Diferente do da biblioteca")
                default:
                    return qsTr("Fora da biblioteca")
                }
            }
            color: palette.placeholderText
            Layout.fillWidth: true
        }

        Button {
            text: qsTr("Salvar na biblioteca")
            enabled: root.typeController.libraryState !== TypeDefinitionController.SameAsLibrary
                     && !root.typeController.modified
            onClicked: root.saveToLibrary()

            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: qsTr("Grava o tipo aplicado, com os tipos e formatos que ele usa, na pasta da biblioteca.")
        }

        Button {
            text: qsTr("Atualizar da biblioteca")
            enabled: root.typeController.libraryState === TypeDefinitionController.DiffersFromLibrary
            onClicked: root.typeController.updateFromLibrary()

            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: qsTr("Troca a definição deste tipo pela da biblioteca. As edições não aplicadas são descartadas.")
        }

        Button {
            text: qsTr("Remover")
            onClicked: removal.open()

            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: qsTr("Remove o tipo do documento. Só é possível se nenhum campo o usa.")
        }
    }

    ConfirmPopup {
        id: overwrite

        title: qsTr("Substituir na biblioteca")
        text: qsTr("A biblioteca já tem o tipo '%1'. Substituir pelo deste documento?").arg(root.current)
        confirmText: qsTr("Substituir")
        onConfirmed: root.app.saveTypeToLibrary(root.current)
    }

    ConfirmPopup {
        id: removal

        title: qsTr("Remover tipo")
        text: qsTr("Remover o tipo '%1' do documento?").arg(root.current)
        confirmText: qsTr("Remover")
        onConfirmed: root.typeController.removeType()
    }

    Label {
        visible: root.typeController.modified
        text: qsTr("Alterações do tipo '%1' ainda não aplicadas").arg(root.typeController.currentType)
        color: palette.placeholderText
        Layout.leftMargin: Theme.pagePadding
        Layout.bottomMargin: Theme.pagePadding
    }

    NewItemPopup {
        id: newType

        title: qsTr("Novo tipo")
        onAccepted: name => root.typeController.createType(name)
    }

    SchemaTreeView {
        id: tree

        visible: root.hasTypes
        model: root.typeController.draftModel
        Layout.fillWidth: true
        Layout.fillHeight: true
    }
}

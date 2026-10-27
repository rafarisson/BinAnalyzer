import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Tipos customizados do documento: o ComboBox escolhe o tipo e a árvore edita os campos dele, com as
// mesmas regras da estrutura. As edições ficam em um rascunho até "Aplicar"; "Restaurar" volta à
// definição do documento. Sem tipos customizados o ComboBox fica desabilitado e a árvore escondida.
ColumnLayout {
    id: root

    required property TypeDefinitionController typeController

    readonly property bool hasTypes: typeController.typeIds.length > 0

    function expandAll() {
        tree.expandAll()
    }

    spacing: 0

    RowLayout {
        id: header

        Layout.fillWidth: true
        Layout.margins: Theme.pagePadding

        Label {
            text: qsTr("Tipos")
            font.bold: true
        }

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

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Pede o nome (e, se pedido, o padrão) de um tipo ou formato novo. A validação é do documento:
// quem recebe accepted() cria o item e mostra a recusa, se houver.
BinaPopup {
    id: root

    property string title
    property bool askPattern: false
    property string patternText: "%d"

    signal accepted(string name, string pattern)

    readonly property bool valid: nameField.text.trim() !== "" && (!askPattern || patternField.text !== "")

    function submit() {
        if (!valid)
            return
        accepted(nameField.text.trim(), patternField.text)
        close()
    }

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(360, Overlay.overlay ? Overlay.overlay.width - 2 * Theme.pagePadding : 360)
    padding: Theme.spacingXl
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onAboutToShow: {
        nameField.text = ""
        patternField.text = patternText
    }
    onOpened: nameField.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingLg

        Label {
            text: root.title
            font.bold: true
            font.pixelSize: Theme.fontTitle
        }

        BinaTextField {
            id: nameField

            placeholderText: qsTr("Nome")
            selectByMouse: true
            Layout.fillWidth: true
            onAccepted: root.submit()
        }

        BinaTextField {
            id: patternField

            visible: root.askPattern
            placeholderText: qsTr("Padrão: %(campo)d, %x ...")
            font.family: Theme.monoFont
            selectByMouse: true
            Layout.fillWidth: true
            onAccepted: root.submit()
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight

            BinaButton {
                text: qsTr("Cancelar")
                onClicked: root.close()
            }

            BinaButton {
                text: qsTr("Criar")
                enabled: root.valid
                highlighted: root.valid
                onClicked: root.submit()
            }
        }
    }
}

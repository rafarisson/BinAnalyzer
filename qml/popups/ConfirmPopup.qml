import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Pede a confirmação de uma ação: confirmed() só é emitido pelo botão de confirmar.
Popup {
    id: root

    property string title
    property string text
    property string confirmText: qsTr("Confirmar")

    signal confirmed()

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(360, Overlay.overlay ? Overlay.overlay.width - 2 * Theme.pagePadding : 360)
    padding: Theme.pagePadding
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.pagePadding

        Label {
            text: root.title
            font.bold: true
        }

        Label {
            text: root.text
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight

            Button {
                text: qsTr("Cancelar")
                onClicked: root.close()
            }

            Button {
                text: root.confirmText
                highlighted: true
                onClicked: {
                    root.close()
                    root.confirmed()
                }
            }
        }
    }
}

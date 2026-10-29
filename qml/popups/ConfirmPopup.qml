import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Pede a confirmação de uma ação: confirmed() só é emitido pelo botão de confirmar.
BinaPopup {
    id: root

    property string title
    property string text
    property string confirmText: qsTr("Confirmar")

    signal confirmed()

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(360, Overlay.overlay ? Overlay.overlay.width - 2 * Theme.pagePadding : 360)
    padding: Theme.spacingXl
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingLg

        Label {
            text: root.title
            font.bold: true
            font.pixelSize: Theme.fontTitle
        }

        Label {
            text: root.text
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight

            BinaButton {
                text: qsTr("Cancelar")
                onClicked: root.close()
            }

            BinaButton {
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

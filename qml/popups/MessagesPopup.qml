import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Histórico de mensagens (avisos do carregamento, recusas de edição e erros).
Popup {
    id: root

    required property AppController app

    // Posiciona o popup logo abaixo do botão, alinhado à direita dele e dentro da janela.
    function placeBelow() {
        const overlay = Overlay.overlay
        if (!anchorItem || !overlay)
            return

        const below = anchorItem.mapToItem(overlay, anchorItem.width, anchorItem.height)
        const limit = Theme.pagePadding

        x = Math.max(limit, Math.min(below.x - width, overlay.width - width - limit))
        y = Math.max(limit, Math.min(below.y, overlay.height - height - limit))
    }

    // O botão que abre o popup: o popup abre logo abaixo dele, alinhado à direita e sem passar das
    // bordas da janela.
    property Item anchorItem

    parent: Overlay.overlay
    width: Math.min(560, Overlay.overlay ? Overlay.overlay.width - 2 * Theme.pagePadding : 560)
    height: Math.min(320, Overlay.overlay ? Overlay.overlay.height - 2 * Theme.pagePadding : 320)
    padding: 8
    margins: Theme.pagePadding
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onAboutToShow: root.placeBelow()

    contentItem: ColumnLayout {
        spacing: 6

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("Mensagens")
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Limpar")
                enabled: root.app.messageLog.count > 0
                onClicked: root.app.clearMessages()
            }
        }

        ListView {
            id: list

            clip: true
            model: root.app.messageLog
            Layout.fillWidth: true
            Layout.fillHeight: true

            delegate: Label {
                required property string display
                required property bool isError

                width: ListView.view.width
                text: display
                color: isError ? Theme.errorText : palette.text
                wrapMode: Text.WordWrap
            }

            // Mostra sempre a mensagem mais nova.
            onCountChanged: Qt.callLater(positionViewAtEnd)

            ScrollBar.vertical: ScrollBar {}
        }
    }
}

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Histórico de mensagens (avisos do carregamento, recusas de edição e erros).
Popup {
    id: root

    required property AppController app

    width: 560
    height: 320
    padding: 8

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

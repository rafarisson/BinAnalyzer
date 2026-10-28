import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Painel vazio com uma orientação e, se houver, a ação para sair dele.
Rectangle {
    id: root

    property string title
    property string hint
    property string actionText

    signal actionRequested()

    color: palette.base

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 4 * Theme.pagePadding, 360)
        spacing: Theme.pagePadding

        Label {
            text: root.title
            font.bold: true
            font.pixelSize: 15
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            visible: root.hint !== ""
            text: root.hint
            color: palette.placeholderText
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Button {
            visible: root.actionText !== ""
            text: root.actionText
            highlighted: true
            Layout.alignment: Qt.AlignHCenter
            onClicked: root.actionRequested()
        }
    }
}

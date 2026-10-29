import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Painel vazio com uma orientação e, se houver, a ação para sair dele.
Rectangle {
    id: root

    property string title
    property string hint
    property string actionText
    property string glyph: Icons.fileTreeOutline

    signal actionRequested()

    color: Theme.surface

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 4 * Theme.pagePadding, 360)
        spacing: Theme.spacingMd

        Rectangle {
            visible: root.glyph !== ""
            radius: width / 2
            color: Qt.alpha(Theme.accent, 0.12)
            implicitWidth: 64
            implicitHeight: 64
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: Theme.spacingSm

            Icon {
                anchors.centerIn: parent
                text: root.glyph
                font.pixelSize: 32
                color: Theme.accent
            }
        }

        Label {
            text: root.title
            font.bold: true
            font.pixelSize: Theme.fontTitle
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            visible: root.hint !== ""
            text: root.hint
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        BinaButton {
            visible: root.actionText !== ""
            text: root.actionText
            highlighted: true
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.spacingSm
            onClicked: root.actionRequested()
        }
    }
}

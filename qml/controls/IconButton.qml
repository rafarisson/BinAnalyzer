import QtQuick
import QtQuick.Controls.Basic

// Botão da barra com um ícone (glifo de Icons) antes do texto, ou só o ícone.
ToolButton {
    id: root

    property string glyph
    property color contentColor: Theme.text

    hoverEnabled: true

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight
        opacity: root.enabled ? 1 : 0.4

        Row {
            id: row

            anchors.centerIn: parent
            spacing: 6

            Icon {
                visible: root.glyph !== ""
                text: root.glyph
                color: root.contentColor
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                visible: root.text !== ""
                text: root.text
                font: root.font
                color: root.contentColor
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    background: Rectangle {
        radius: Theme.radiusMedium
        color: !root.enabled ? "transparent"
               : root.down ? Theme.pressedOverlay
               : root.hovered ? Theme.hoverOverlay
               : "transparent"
        border.width: root.visualFocus ? 2 : 0
        border.color: Qt.alpha(Theme.accent, 0.6)

        Behavior on color {
            ColorAnimation {
                duration: Theme.animFast
            }
        }
    }
}

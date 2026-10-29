import QtQuick
import QtQuick.Controls.Basic

// Um segmento de SegmentedControl: ícone (glifo de Icons) e texto. Só as pontas do bloco são arredondadas.
Button {
    id: root

    property string glyph

    readonly property bool isFirst: !!parent && parent.children[0] === root
    readonly property bool isLast: !!parent && parent.children[parent.children.length - 1] === root

    checkable: true
    height: parent ? parent.height : implicitHeight
    implicitWidth: Math.max(64, implicitContentWidth + 24)
    width: parent && parent.stretch ? parent.width / parent.children.length : implicitWidth
    hoverEnabled: true

    contentItem: Item {
        implicitWidth: content.implicitWidth
        implicitHeight: content.implicitHeight

        Row {
            id: content

            anchors.centerIn: parent
            spacing: Theme.spacingSm

            Icon {
                visible: root.glyph !== ""
                text: root.glyph
                font.pixelSize: 16
                color: root.checked ? Theme.accentText : Theme.text
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: root.text
                font: root.font
                color: root.checked ? Theme.accentText : Theme.text
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    background: Rectangle {
        radius: Theme.radiusMedium
        color: root.checked ? Theme.accent
               : root.down ? Theme.controlPressed
               : root.hovered ? Theme.controlHover
               : Theme.control

        Behavior on color {
            ColorAnimation {
                duration: Theme.animFast
            }
        }

        // Os cantos internos são retos.
        Rectangle {
            visible: !root.isFirst
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.radius
            color: parent.color
        }

        Rectangle {
            visible: !root.isLast
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.radius
            color: parent.color
        }
    }
}

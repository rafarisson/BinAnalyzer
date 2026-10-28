import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Alterna entre a visão em árvore e a visão em tabela (controle segmentado).
RowLayout {
    id: root

    readonly property bool tableMode: tableButton.checked

    spacing: 0

    ButtonGroup {
        id: group
    }

    component Segment: Button {
        id: segment

        property int position: 0

        checkable: true
        implicitHeight: 26
        implicitWidth: Math.max(64, implicitContentWidth + 24)
        ButtonGroup.group: group

        property string glyph

        contentItem: Row {
            spacing: 6

            Icon {
                text: segment.glyph
                color: segment.checked ? Theme.accentText : segment.palette.buttonText
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                text: segment.text
                font.bold: segment.checked
                color: segment.checked ? Theme.accentText : segment.palette.buttonText
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        background: Rectangle {
            radius: 4
            color: segment.checked ? Theme.accent : segment.hovered ? segment.palette.midlight : segment.palette.button
            border.color: segment.checked ? Theme.accent : Theme.headerBorder

            // Os cantos internos são retos: o par parece um bloco só.
            Rectangle {
                visible: segment.position !== 0
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.radius
                color: parent.color
            }

            Rectangle {
                visible: segment.position === 0
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.radius
                color: parent.color
            }
        }
    }

    Segment {
        id: treeButton

        text: qsTr("Árvore")
        glyph: Icons.fileTree
        checked: true
        position: 0
    }

    Segment {
        id: tableButton

        text: qsTr("Tabela")
        glyph: Icons.tableLarge
        position: 1
    }

    Shortcut {
        sequence: "Ctrl+1"
        onActivated: treeButton.checked = true
    }

    Shortcut {
        sequence: "Ctrl+2"
        onActivated: tableButton.checked = true
    }
}

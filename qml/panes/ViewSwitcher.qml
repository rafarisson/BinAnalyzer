import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Alterna entre a visão em árvore e a visão em tabela (controle segmentado).
SegmentedControl {
    id: root

    readonly property bool tableMode: tableButton.checked

    SegmentedButton {
        id: treeButton

        text: qsTr("Árvore")
        glyph: Icons.fileTree
        checked: true
    }

    SegmentedButton {
        id: tableButton

        text: qsTr("Tabela")
        glyph: Icons.tableLarge
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

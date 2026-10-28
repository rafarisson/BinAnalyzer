import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Botões de expandir e recolher a árvore de um painel.
RowLayout {
    signal expandRequested()
    signal collapseRequested()

    spacing: 2

    IconButton {
        glyph: Icons.expandAll
        implicitWidth: 32
        implicitHeight: 28
        ToolTip.visible: hovered
        ToolTip.delay: 500
        ToolTip.text: qsTr("Expandir tudo")
        onClicked: expandRequested()
    }

    IconButton {
        glyph: Icons.collapseAll
        implicitWidth: 32
        implicitHeight: 28
        ToolTip.visible: hovered
        ToolTip.delay: 500
        ToolTip.text: qsTr("Recolher tudo")
        onClicked: collapseRequested()
    }
}

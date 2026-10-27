import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Alterna entre a visão em árvore e a visão em tabela.
RowLayout {
    id: root

    readonly property bool tableMode: tableButton.checked

    ButtonGroup {
        id: group
    }

    Button {
        text: qsTr("Árvore")
        checkable: true
        checked: true
        ButtonGroup.group: group
    }

    Button {
        id: tableButton

        text: qsTr("Tabela")
        checkable: true
        ButtonGroup.group: group
    }
}

import QtQuick

// Fundo de uma célula de tabela ou árvore. A linha selecionada tem fundo na cor de destaque e um
// contorno na linha inteira: cada célula desenha a parte dela (as bordas de cima e de baixo, e a lateral
// só nas pontas). As demais linhas alternam as cores do tema e ganham um realce sob o mouse.
Rectangle {
    id: root

    property bool selected: false
    property int row: 0
    property int column: 0
    property int columns: 1
    property bool alternating: true

    // O mouse está sobre a linha (informado por quem desenha a célula).
    property bool hovered: false

    color: selected ? Theme.selectionFill
           : hovered ? Theme.rowHover
           : alternating && row % 2 === 1 ? Theme.surfaceAlt
           : Theme.surface

    Rectangle {
        visible: root.selected
        anchors.fill: parent
        color: "transparent"
        border.color: Theme.selectionBorder
        border.width: 1

        // Esconde a lateral das células do meio: o contorno só fecha nas pontas.
        Rectangle {
            visible: root.column > 0
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 1
            anchors.bottomMargin: 1
            width: 1
            color: Theme.selectionFill
        }

        Rectangle {
            visible: root.column < root.columns - 1
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 1
            anchors.bottomMargin: 1
            width: 1
            color: Theme.selectionFill
        }
    }
}

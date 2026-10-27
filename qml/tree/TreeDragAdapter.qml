import QtQuick

import Bina.App

// Adaptador de arrastar e soltar para um TreeView cujo model tem canDropAt(origem, alvo, zona) e
// dropAt(origem, alvo, zona). Cobre exatamente a área da view e não captura o mouse: quem inicia o
// arrasto (um DragHandler no delegate) chama begin(), move() e end().
//
// Todas as regras de soltura ficam no model; este item só traduz a posição do ponteiro em
// (alvo, zona) e desenha o indicador. Se a view passar a oferecer arrastar linhas nativamente,
// este componente e os handlers dos delegates podem ser removidos.
Item {
    id: root

    required property TreeView view    // o TreeView
    required property SchemaTreeModel itemModel

    property bool dragging: false
    property var source
    property var target
    property int zone: SchemaTreeModel.Onto
    property bool valid: false

    function begin(sourceIndex, text) {
        source = sourceIndex
        target = undefined
        ghost.text = text
        valid = false
        dragging = true
    }

    function move(scenePosition) {
        const point = root.mapFromItem(null, scenePosition)

        ghost.x = point.x + 12
        ghost.y = point.y + 8

        valid = false

        const cell = view.cellAtPosition(view.mapFromItem(null, scenePosition), true)
        const item = cell.y >= 0 ? view.itemAtCell(Qt.point(0, cell.y)) as SchemaDelegate : null
        if (!item)
            return

        const top = item.mapToItem(root, 0, 0).y
        const fraction = (point.y - top) / item.height

        zone = fraction < 0.25 ? SchemaTreeModel.Before
               : fraction > 0.75 ? SchemaTreeModel.After : SchemaTreeModel.Onto
        target = view.index(cell.y, 0)
        indicator.leftInset = 0

        // Sobre a borda de baixo de um grupo aberto, o ponteiro à direita do início do nome
        // "entra" no grupo (antes do primeiro campo); à esquerda, fica depois dele como irmão.
        if (zone === SchemaTreeModel.After && view.isExpanded(cell.y)
                && point.x >= item.contentItem.mapToItem(root, 0, 0).x) {
            let row = cell.y + 1
            let first = view.itemAtCell(Qt.point(0, row)) as SchemaDelegate
            while (first && first.isAutoPad)
                first = view.itemAtCell(Qt.point(0, ++row)) as SchemaDelegate

            if (first && first.depth > item.depth) {
                target = view.index(row, 0)
                zone = SchemaTreeModel.Before
                indicator.leftInset = first.contentItem.mapToItem(root, 0, 0).x
                indicator.rowTop = first.mapToItem(root, 0, 0).y
                indicator.rowHeight = first.height
                valid = itemModel.canDropAt(source, target, zone)
                return
            }
        }

        valid = itemModel.canDropAt(source, target, zone)

        indicator.rowTop = top
        indicator.rowHeight = item.height
    }

    function end() {
        if (dragging && valid)
            itemModel.dropAt(source, target, zone)

        dragging = false
        valid = false
    }

    DropIndicator {
        id: indicator

        visible: root.dragging && root.target !== undefined
        z: 10
        width: root.width - indicator.leftInset
        zone: root.zone
        valid: root.valid
    }

    DragGhost {
        id: ghost

        visible: root.dragging
        z: 11
    }
}

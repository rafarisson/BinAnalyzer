import QtQuick
import QtQuick.Controls.Basic

// TreeView que sabe qual linha está em destaque: a da célula atual, seja qual for a coluna clicada.
// Os delegates comparam a própria linha com highlightedRow, em vez de depender da seleção do Qt (cada
// delegate a altera de um jeito: só a célula, a linha inteira ou nada).
TreeView {
    id: root

    // Linha (da view) da célula atual; -1 sem célula atual.
    property int highlightedRow: -1

    // Linha sob o mouse; -1 com o mouse fora da view.
    property int hoveredRow: -1

    function syncHoveredRow() {
        if (!hover.hovered) {
            root.hoveredRow = -1
            return
        }

        root.hoveredRow = root.cellAtPosition(hover.point.position).y
    }

    HoverHandler {
        id: hover

        onPointChanged: root.syncHoveredRow()
        onHoveredChanged: root.syncHoveredRow()
    }

    ScrollBar.vertical: BinaScrollBar {}
    ScrollBar.horizontal: BinaScrollBar {}

    function syncHighlightedRow() {
        const index = root.selectionModel ? root.selectionModel.currentIndex : null

        root.highlightedRow = index && index.valid ? root.rowAtIndex(index) : -1
    }

    // Abrir e fechar grupos desloca as linhas.
    onRowsChanged: {
        syncHighlightedRow()
        Qt.callLater(syncHighlightedRow)
    }

    Connections {
        target: root.selectionModel

        function onCurrentChanged() {
            root.syncHighlightedRow()
        }
    }
}

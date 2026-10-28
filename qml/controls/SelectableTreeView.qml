import QtQuick

// TreeView que sabe qual linha está em destaque: a da célula atual, seja qual for a coluna clicada.
// Os delegates comparam a própria linha com highlightedRow, em vez de depender da seleção do Qt (cada
// delegate a altera de um jeito: só a célula, a linha inteira ou nada).
TreeView {
    id: root

    // Linha (da view) da célula atual; -1 sem célula atual.
    property int highlightedRow: -1

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

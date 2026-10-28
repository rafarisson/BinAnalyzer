import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Árvore dos dados analisados (um nó por campo do binário).
Item {
    id: root

    required property ResultTreeModel model
    required property AnalysisSession session

    // A árvore do cabeçalho: um único registro, sem os detalhes do campo.
    property bool header: false

    readonly property bool manyRecords: !header && session.recordCount > 1

    // Com vários registros, "Expandir tudo" abre só os registros visíveis e mais uma página, e
    // continua abrindo os que aparecem ao rolar: abrir de uma vez milhares de registros (cada grupo
    // aberto é inserido um a um no TreeView) trava a janela.
    property bool expandEverything: false
    property var expandedRecords: ({})

    readonly property int expandBatch: 50

    function expandAll() {
        if (root.manyRecords) {
            expandEverything = true
            expandVisible()
        } else {
            tree.expandRecursively()
        }
    }

    function collapseAll() {
        expandEverything = false
        expandedRecords = {}
        tree.collapseRecursively()
    }

    function expandVisible() {
        if (!expandEverything || tree.rows === 0 || tree.topRow < 0)
            return

        const page = Math.max(1, tree.bottomRow - tree.topRow + 1)
        let expanded = 0

        for (let row = tree.topRow; row < tree.rows && row <= tree.bottomRow + page && expanded < expandBatch; ++row) {
            if (tree.depth(row) !== 0)
                continue

            const record = tree.index(row, 0).row
            if (expandedRecords[record])
                continue

            expandedRecords[record] = true
            tree.expandRecursively(row)
            ++expanded
        }
    }

    function resetExpansion() {
        expandedRecords = {}
        expandInitial()
        Qt.callLater(expandVisible)
    }

    // Um registro é mostrado todo aberto; vários registros começam com os campos de cada registro
    // visíveis e os grupos internos fechados.
    function expandInitial() {
        if (root.manyRecords)
            tree.expandRecursively(-1, 1)
        else
            tree.expandRecursively()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        HorizontalHeaderView {
            syncView: tree
            clip: true
            Layout.fillWidth: true
        }

        SelectableTreeView {
            id: tree

            clip: true
            alternatingRows: true
            model: root.model

            // Os registros vêm em lotes. O TreeView não repassa o pedido de mais linhas ao model ao
            // chegar no fim (só a TableView faz isso), então o pedido é feito aqui.
            onAtYEndChanged: if (atYEnd) root.model.fetchMoreRecords()

            // Rolar ou carregar mais registros abre os novos visíveis, no modo "Expandir tudo".
            onTopRowChanged: Qt.callLater(root.expandVisible)
            onBottomRowChanged: Qt.callLater(root.expandVisible)
            selectionModel: ItemSelectionModel {
                id: selection
            }
            selectionBehavior: TableView.SelectRows
            editTriggers: TableView.DoubleTapped | TableView.EditKeyPressed
            columnWidthProvider: function (column) {
                return Theme.resultColumnWidths[column] ?? Theme.defaultColumnWidth
            }
            delegate: ResultTreeDelegate {}
            Layout.fillWidth: true
            Layout.fillHeight: true

            Component.onCompleted: Qt.callLater(root.expandInitial)
        }

        ResultDetails {
            model: root.model
            currentIndex: selection.currentIndex
            visible: root.session.hasData && !root.header
            Layout.fillWidth: true
        }
    }

    // Uma troca de estrutura recria as linhas; mantém a árvore aberta (e o modo "Expandir tudo").
    Connections {
        target: root.model

        function onModelReset() {
            Qt.callLater(root.resetExpansion)
        }
    }
}

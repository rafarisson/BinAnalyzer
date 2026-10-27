import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Árvore dos dados analisados (um nó por campo do binário).
Item {
    id: root

    required property ResultTreeModel model
    required property AnalysisSession session

    function expandAll() {
        tree.expandRecursively()
    }

    // Um registro é mostrado todo aberto; vários registros começam com os campos de cada registro
    // visíveis e os grupos internos fechados.
    function expandInitial() {
        if (root.session.recordCount > 1)
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

        TreeView {
            id: tree

            clip: true
            alternatingRows: true
            model: root.model

            // Os registros vêm em lotes. O TreeView não repassa o pedido de mais linhas ao model ao
            // chegar no fim (só a TableView faz isso), então o pedido é feito aqui.
            onAtYEndChanged: if (atYEnd) root.model.fetchMoreRecords()
            selectionModel: ItemSelectionModel {}
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
    }

    // Uma troca de estrutura recria as linhas; mantém a árvore aberta.
    Connections {
        target: root.model

        function onModelReset() {
            Qt.callLater(root.expandInitial)
        }
    }
}

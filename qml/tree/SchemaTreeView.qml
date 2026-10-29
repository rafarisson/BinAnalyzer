pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Árvore editável da estrutura. Edição por coluna, menu de contexto e arrastar e soltar vêm do
// model; aqui ficam os gestos (via SchemaDelegate), o desenho e os atalhos de teclado.
Item {
    id: root

    required property SchemaTreeModel model

    readonly property bool menuOpen: contextMenu.visible

    function expandAll() {
        tree.expandRecursively()
    }

    function collapseAll() {
        tree.collapseRecursively()
    }

    // Chamados pelos delegates (coluna do nome).
    function openMenu(index, position) {
        contextMenu.openFor(index, root, position.x, position.y)
    }

    function beginDrag(index, text) {
        dragAdapter.begin(index, text)
    }

    function dragMoved(scenePosition) {
        dragAdapter.move(scenePosition)
    }

    function endDrag() {
        dragAdapter.end()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        HorizontalHeaderView {
            syncView: tree
            clip: true
            delegate: ColumnHeaderCell {}
            Layout.fillWidth: true
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Clicar na árvore leva o foco do teclado para ela (atalhos abaixo).
            TapHandler {
                onTapped: tree.forceActiveFocus()
            }

            SelectableTreeView {
                id: tree

                anchors.fill: parent
                clip: true
                alternatingRows: true
                model: root.model
                selectionModel: ItemSelectionModel {}
                selectionBehavior: TableView.SelectRows
                editTriggers: TableView.DoubleTapped | TableView.EditKeyPressed
                columnWidthProvider: function (column) {
                    return Theme.schemaColumnWidths[column] ?? Theme.defaultColumnWidth
                }

                delegate: SchemaDelegate {
                    controller: root
                }

                Component.onCompleted: Qt.callLater(expandRecursively)
            }

            EmptyState {
                visible: tree.rows === 0
                anchors.fill: parent
                title: qsTr("Nenhum campo ainda")
                hint: qsTr("Adicione o primeiro campo. Depois, clique com o botão direito num campo para inserir, remover ou arrastar.")
                actionText: qsTr("Adicionar campo")
                onActionRequested: root.model.trigger(root.model.index(-1, -1), "insertInside")
            }

            TreeDragAdapter {
                id: dragAdapter

                anchors.fill: parent
                view: tree
                itemModel: root.model
            }
        }
    }

    TreeContextMenu {
        id: contextMenu

        itemModel: root.model

        onTextRequested: (index, actionId, prompt) => {
            textInput.targetIndex = index
            textInput.actionId = actionId
            textInput.title = prompt
            textInput.open()
        }
    }

    // O texto de uma ação do menu (o id do tipo novo, por exemplo).
    NewItemPopup {
        id: textInput

        property var targetIndex
        property string actionId

        onAccepted: name => root.model.trigger(targetIndex, actionId, name)
    }

    Connections {
        target: root.model

        // Uma estrutura nova (abrir arquivo) volta a aparecer aberta.
        function onModelReset() {
            Qt.callLater(tree.expandRecursively)
        }

        // Campo criado ou movido: abre os pais, rola até ele e o seleciona.
        function onRevealRequested(index) {
            tree.expandToIndex(index)

            Qt.callLater(() => {
                const row = tree.rowAtIndex(index)

                if (row >= 0) {
                    tree.positionViewAtRow(row, Qt.AlignVCenter)
                    tree.selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect)
                }
            })
        }
    }

    Shortcut {
        sequences: ["Insert"]
        enabled: tree.activeFocus && tree.selectionModel.currentIndex.valid
        onActivated: root.model.trigger(tree.selectionModel.currentIndex, "insertBelow")
    }

    Shortcut {
        sequences: ["Delete"]
        enabled: tree.activeFocus && tree.selectionModel.currentIndex.valid
        onActivated: root.model.trigger(tree.selectionModel.currentIndex, "remove")
    }

    Shortcut {
        sequences: ["F2"]
        enabled: tree.activeFocus && tree.selectionModel.currentIndex.valid
        onActivated: tree.edit(tree.index(tree.rowAtIndex(tree.selectionModel.currentIndex), 0))
    }
}

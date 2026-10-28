pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

// Célula da árvore da estrutura. A edição de cada coluna vem do model (ver CellEditDelegate).
// Na coluna do nome ficam também o menu de contexto e o arrasto, que o 'controller' (a
// SchemaTreeView) executa; auto_pad não participa de nenhum dos dois.
TreeViewDelegate {
    id: cell

    required property int column
    required property bool isAutoPad

    property SchemaTreeView controller

    readonly property bool fieldEditable: column === 0 && !isAutoPad && !(controller && controller.menuOpen)

    // Campo com o formato "hidden": esmaecido, como o auto_pad.
    readonly property bool hiddenValue: cell.model && cell.model.formatId === Theme.hiddenFormat

    implicitHeight: Theme.rowHeight

    // O mesmo fundo da tabela: linhas alternadas e a linha selecionada com contorno.
    background: RowBackground {
        selected: cell.row === cell.treeView.highlightedRow
        row: cell.row
        column: cell.column
        columns: cell.treeView.columns
        alternating: cell.treeView.alternatingRows
    }

    contentItem: Label {
        text: cell.model && cell.model.display !== undefined ? cell.model.display : ""
        elide: Text.ElideRight
        opacity: cell.isAutoPad || cell.hiddenValue ? 0.55 : 1
        font.italic: cell.isAutoPad || cell.hiddenValue
        color: cell.column === 1 ? Theme.typeColor(text, cell.palette.buttonText)
               : cell.palette.buttonText
        visible: !cell.editing
    }

    TableView.editDelegate: CellEditDelegate {
        width: parent.width
        height: parent.height
        view: cell.treeView
        itemIndex: cell.treeView.index(cell.row, cell.column)
        cellData: cell.model
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        enabled: cell.fieldEditable
        onTapped: point => cell.controller.openMenu(cell.treeView.index(cell.row, 0),
                                                      cell.mapToItem(cell.controller, point.position))
    }

    // PointHandler só observa (grab passivo), então clique, seleção e edição continuam chegando ao
    // delegate; o arrasto começa depois de passar do limiar.
    PointHandler {
        id: dragWatcher

        property bool dragging: false

        enabled: cell.fieldEditable

        onPointChanged: {
            if (!active)
                return
            if (!dragging) {
                const dx = point.scenePosition.x - point.scenePressPosition.x
                const dy = point.scenePosition.y - point.scenePressPosition.y
                if (Math.abs(dx) + Math.abs(dy) < 10)
                    return
                dragging = true
                cell.controller.beginDrag(cell.treeView.index(cell.row, 0), cell.model.display)
            }
            cell.controller.dragMoved(point.scenePosition)
        }

        // Enquanto o botão está pressionado a view não pode rolar com o arrasto.
        onActiveChanged: {
            cell.treeView.interactive = !active
            if (!active && dragging) {
                dragging = false
                cell.controller.endDrag()
            }
        }
    }

    ToolTip.visible: hovered && isAutoPad
    ToolTip.delay: 500
    ToolTip.text: qsTr("Preenchimento automático para manter o alinhamento. Não pode ser editado.")
}

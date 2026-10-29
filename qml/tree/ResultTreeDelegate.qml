pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

// Célula da árvore dos dados analisados. Só a coluna "Exibição" é editável; o editor vem do model.
TreeViewDelegate {
    id: cell

    required property int column
    required property bool truncated

    implicitHeight: Theme.rowHeight

    // O mesmo fundo da tabela: linhas alternadas e a linha selecionada com contorno.
    background: RowBackground {
        selected: cell.row === cell.treeView.highlightedRow
        hovered: cell.row === cell.treeView.hoveredRow
        row: cell.row
        column: cell.column
        columns: cell.treeView.columns
        alternating: cell.treeView.alternatingRows
    }

    contentItem: Label {
        text: cell.model && cell.model.display !== undefined ? cell.model.display : ""
        elide: Text.ElideRight
        // Fallback na fonte do delegate (nunca alterada): o próprio font.family deste Label guardaria
        // a fonte mono de quando o delegate foi reciclado de outra coluna.
        font.family: cell.column >= 2 ? Theme.monoFont : cell.font.family
        opacity: cell.truncated ? Theme.dimmedOpacity : 1
        color: cell.column === 1 ? Theme.typeColor(text, Theme.text)
               : Theme.text
        horizontalAlignment: cell.column === 3 || cell.column === 2 ? Text.AlignRight : Text.AlignLeft
        visible: !cell.editing
    }

    TableView.editDelegate: CellEditDelegate {
        width: parent.width
        height: parent.height
        view: cell.treeView
        itemIndex: cell.treeView.index(cell.row, cell.column)
        cellData: cell.model
    }
}

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import Bina.App

// Os dados analisados em tabela: um registro por linha e um campo por coluna, já formatados.
// O botão direito numa célula abre um menu para trocar o formato de exibição do campo.
Item {
    id: root

    required property ResultTableModel model

    function openFormatMenu(row, column, currentId, position) {
        const index = table.index(row, column)
        const formats = root.model.compatibleFormats(index)

        formatMenu.targetIndex = index
        formatMenu.currentId = currentId
        formatMenu.choices = [{ "id": "", "label": qsTr("(padrão)") }].concat(
            formats.map(id => ({ "id": id, "label": id })))
        formatMenu.popup(root, position.x, position.y)
    }

    SystemPalette {
        id: colors
    }

    HorizontalHeaderView {
        id: columnHeader

        syncView: table
        anchors.left: table.left
        anchors.top: parent.top
        clip: true
    }

    VerticalHeaderView {
        id: rowHeader

        syncView: table
        anchors.left: parent.left
        anchors.top: table.top
        clip: true
    }

    TableView {
        id: table

        anchors.left: rowHeader.right
        anchors.top: columnHeader.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        columnSpacing: 1
        rowSpacing: 1
        model: root.model
        selectionModel: ItemSelectionModel {}
        editTriggers: TableView.DoubleTapped | TableView.EditKeyPressed
        columnWidthProvider: function (column) {
            const header = String(root.model.headerData(column, Qt.Horizontal, Qt.DisplayRole))
            return Math.max(120, header.length * 8 + 24)
        }

        ScrollBar.horizontal: ScrollBar {}
        ScrollBar.vertical: ScrollBar {}

        delegate: Rectangle {
            id: cell

            required property int row
            required property int column
            required property string display
            required property string formatId
            required property bool truncated
            required property bool editing
            required property var model

            implicitWidth: 120
            implicitHeight: Theme.rowHeight
            color: row % 2 === 0 ? colors.base : colors.alternateBase

            Accessible.role: Accessible.Cell
            Accessible.name: cell.display

            Label {
                anchors.fill: parent
                anchors.leftMargin: Theme.cellPadding
                anchors.rightMargin: Theme.cellPadding
                verticalAlignment: Text.AlignVCenter
                text: cell.display
                elide: Text.ElideRight
                font.family: Theme.monoFont
                opacity: cell.truncated ? Theme.dimmedOpacity : 1
                visible: !cell.editing
            }

            // O valor abre num editor só para ser selecionado e copiado; o model recusa a gravação.
            TableView.editDelegate: CellEditDelegate {
                width: parent.width
                height: parent.height
                view: table
                itemIndex: table.index(cell.row, cell.column)
                cellData: cell.model
            }

            HoverHandler {
                id: hover
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: point => root.openFormatMenu(cell.row, cell.column, cell.formatId,
                                                         cell.mapToItem(root, point.position))
            }

            ToolTip.visible: hover.hovered && cell.formatId !== ""
            ToolTip.delay: 600
            ToolTip.text: qsTr("Exibição: %1 (botão direito para trocar)").arg(cell.formatId)
        }
    }

    Menu {
        id: formatMenu

        property var targetIndex
        property string currentId
        property list<var> choices

        Instantiator {
            model: formatMenu.choices

            delegate: MenuItem {
                required property var modelData

                text: modelData.label
                checkable: true
                checked: modelData.id === formatMenu.currentId
                onTriggered: root.model.setFormat(formatMenu.targetIndex, modelData.id)
            }

            onObjectAdded: (index, object) => formatMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => formatMenu.removeItem(object)
        }
    }
}

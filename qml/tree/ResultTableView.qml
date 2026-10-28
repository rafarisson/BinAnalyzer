pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import Bina.App

// Os dados analisados em tabela: um registro por linha e um campo por coluna, já formatados.
// O botão direito numa célula abre um menu para trocar o formato de exibição do campo; no título de uma
// coluna, um menu para mostrar e esconder colunas.
Item {
    id: root

    required property ResultTableModel model

    // Colunas escondidas, por índice. Uma coluna sem largura não é desenhada.
    property var hiddenColumns: ({})

    readonly property int hiddenCount: Object.keys(hiddenColumns).length

    function setColumnsHidden(columns, hidden) {
        const next = Object.assign({}, hiddenColumns)

        for (const column of columns) {
            if (hidden)
                next[column] = true
            else
                delete next[column]
        }

        hiddenColumns = next
    }

    function setColumnHidden(column, hidden) {
        setColumnsHidden([column], hidden)
    }

    // Itens e separadores são criados sem pai: o addItem() os coloca no menu.
    function addMenuAction(menu, text, enabled, handler) {
        const item = columnItemComponent.createObject(null, { "text": text, "enabled": enabled })

        item.triggered.connect(handler)
        menu.addItem(item)
    }

    // "Mostrar tudo" e uma entrada marcável por coluna (marcada quando visível).
    function fillColumnsMenu(menu) {
        addMenuAction(menu, qsTr("Mostrar tudo"), hiddenCount > 0, () => root.showAllColumns())
        menu.addItem(columnSeparatorComponent.createObject(null))

        for (let column = 0; column < table.columns; ++column) {
            const index = column
            const isHidden = hiddenColumns[index] === true
            const item = columnItemComponent.createObject(null, {
                "text": String(root.model.headerData(index, Qt.Horizontal, Qt.DisplayRole)),
                "checkable": true,
                "checked": !isHidden,
                // Sempre sobra uma coluna visível: sem colunas não haveria título para abrir este menu.
                "enabled": isHidden || table.columns - hiddenCount > 1
            })

            // O clique já alternou o item: marcado quer dizer visível.
            item.triggered.connect(() => root.setColumnHidden(index, !item.checked))
            menu.addItem(item)
        }
    }

    // O menu é montado a cada abertura, com o estado atual das colunas, e descartado ao fechar.
    function openColumnsMenu(parentItem, position) {
        const menu = columnMenuComponent.createObject(root)

        fillColumnsMenu(menu)
        menu.closed.connect(() => Qt.callLater(() => menu.destroy()))
        menu.popup(parentItem, position.x, position.y)
    }

    function showAllColumns() {
        hiddenColumns = ({})
    }

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

        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: point => root.openColumnsMenu(columnHeader, point.position)
        }
    }

    VerticalHeaderView {
        id: rowHeader

        syncView: table

        // Clicar no rótulo da linha (registro[n]) a seleciona; a linha atual também o destaca.
        delegate: Rectangle {
            id: label

            required property var model
            required property int row

            implicitWidth: labelText.implicitWidth + 2 * Theme.cellPadding
            implicitHeight: Theme.rowHeight
            color: label.row === table.highlightedRow ? Theme.selectionFill : colors.window

            Label {
                id: labelText

                anchors.centerIn: parent
                text: String(label.model.display)
                font.bold: label.row === table.highlightedRow
            }

            TapHandler {
                onTapped: table.selectionModel.setCurrentIndex(table.index(label.row, 0), ItemSelectionModel.ClearAndSelect)
            }
        }
        anchors.left: parent.left
        anchors.top: table.top
        clip: true
    }

    TableView {
        id: table

        // Linha da célula atual: é a que fica em destaque, seja qual for a coluna clicada, também
        // depois de editar a célula.
        readonly property int highlightedRow: selectionModel.currentIndex.valid ? selectionModel.currentIndex.row : -1

        anchors.left: rowHeader.right
        anchors.top: columnHeader.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        columnSpacing: 0
        rowSpacing: 0
        model: root.model
        selectionModel: ItemSelectionModel {}
        selectionBehavior: TableView.SelectRows
        editTriggers: TableView.DoubleTapped | TableView.EditKeyPressed

        // Uma função nova a cada mudança das colunas escondidas: o Qt só pede as larguras de novo
        // quando o provedor muda (forceLayout() sozinho reaproveita as larguras já calculadas).
        columnWidthProvider: {
            const hidden = root.hiddenColumns

            return function (column) {
                if (hidden[column])
                    return 0

                const header = String(root.model.headerData(column, Qt.Horizontal, Qt.DisplayRole))
                return Math.max(120, header.length * 8 + 24)
            }
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
            color: "transparent"

            RowBackground {
                anchors.fill: parent
                selected: cell.row === table.highlightedRow
                row: cell.row
                column: cell.column
                columns: table.columns
            }

            // Separador fino entre colunas (a tabela não tem espaço entre as células).
            Rectangle {
                visible: cell.row !== table.highlightedRow
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: Theme.headerBorder
                opacity: 0.5
            }

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

    // Uma estrutura nova recria as colunas: tudo volta a aparecer.
    Connections {
        target: root.model

        function onModelReset() {
            root.hiddenColumns = ({})
        }
    }

    Component {
        id: columnMenuComponent

        Menu {}
    }

    Component {
        id: columnItemComponent

        MenuItem {}
    }

    Component {
        id: columnSeparatorComponent

        MenuSeparator {}
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

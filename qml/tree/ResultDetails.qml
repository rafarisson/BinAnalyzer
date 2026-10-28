import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Detalhes do campo selecionado na árvore dos dados: caminho, tipo, offset, tamanho, exibição e valor.
// A faixa recolhe pelo cabeçalho.
Rectangle {
    id: root

    required property ResultTreeModel model
    property var currentIndex

    // Colunas do model (ver ResultTreeModel).
    readonly property int nameColumn: 0
    readonly property int typeColumn: 1
    readonly property int offsetColumn: 2
    readonly property int sizeColumn: 3
    readonly property int formatColumn: 4
    readonly property int valueColumn: 5

    property bool expanded: true
    property var info: ({})

    function cell(index, column) {
        const value = root.model.data(root.model.sibling(index.row, column, index), Qt.DisplayRole)
        return value === undefined || value === null ? "" : String(value)
    }

    function refresh() {
        const index = root.currentIndex

        if (index === undefined || index === null || !index.valid) {
            info = ({})
            return
        }

        const path = []
        for (let node = index; node.valid; node = node.parent)
            path.unshift(cell(node, nameColumn))

        info = {
            "path": path.join(" › "),
            "type": cell(index, typeColumn),
            "offset": cell(index, offsetColumn),
            "size": cell(index, sizeColumn),
            "format": cell(index, formatColumn),
            "value": cell(index, valueColumn)
        }
    }

    onCurrentIndexChanged: refresh()

    color: palette.window
    implicitHeight: expanded ? 138 : Theme.headerHeight - 6
    clip: true

    Behavior on implicitHeight {
        NumberAnimation {
            duration: 120
        }
    }

    Connections {
        target: root.model

        function onDataChanged() {
            root.refresh()
        }

        function onModelReset() {
            root.info = ({})
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        color: Theme.headerBorder
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: 4

        RowLayout {
            Layout.fillWidth: true

            Row {
                spacing: 4

                Icon {
                    text: root.expanded ? Icons.chevronDown : Icons.chevronRight
                    anchors.verticalCenter: parent.verticalCenter
                }

                Label {
                    text: qsTr("Detalhes")
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }

                TapHandler {
                    onTapped: root.expanded = !root.expanded
                }
            }

            Label {
                visible: root.info.path !== undefined
                text: root.info.path ?? ""
                color: palette.placeholderText
                elide: Text.ElideLeft
                Layout.fillWidth: true
            }

            Item {
                visible: root.info.path === undefined
                Layout.fillWidth: true
            }
        }

        GridLayout {
            visible: root.expanded && root.info.path !== undefined
            columns: 4
            columnSpacing: Theme.pagePadding * 2
            rowSpacing: 2
            Layout.fillWidth: true

            Label { text: qsTr("Tipo"); color: palette.placeholderText }
            Label { text: root.info.type ?? ""; color: Theme.typeColor(text, palette.text); font.family: Theme.monoFont }
            Label { text: qsTr("Offset"); color: palette.placeholderText }
            Label { text: root.info.offset ?? ""; font.family: Theme.monoFont }

            Label { text: qsTr("Bytes"); color: palette.placeholderText }
            Label { text: root.info.size ?? ""; font.family: Theme.monoFont }
            Label { text: qsTr("Exibição"); color: palette.placeholderText }
            Label { text: root.info.format ?? ""; font.family: Theme.monoFont }
        }

        RowLayout {
            visible: root.expanded && root.info.path !== undefined
            Layout.fillWidth: true

            Label {
                text: qsTr("Valor")
                color: palette.placeholderText
            }

            TextField {
                id: valueField

                readOnly: true
                selectByMouse: true
                text: root.info.value ?? ""
                font.family: Theme.monoFont
                Layout.fillWidth: true
            }

            IconButton {
                glyph: Icons.contentCopy
                text: qsTr("Copiar")
                onClicked: {
                    valueField.selectAll()
                    valueField.copy()
                    valueField.deselect()
                }
            }
        }

        Label {
            visible: root.expanded && root.info.path === undefined
            text: qsTr("Selecione um campo para ver os detalhes.")
            color: palette.placeholderText
        }

        Item {
            Layout.fillHeight: true
        }
    }
}

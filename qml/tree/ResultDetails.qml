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

    color: Theme.surface
    implicitHeight: expanded ? 132 : Theme.headerHeight - 6
    clip: true

    Behavior on implicitHeight {
        NumberAnimation {
            duration: Theme.animNormal
            easing.type: Easing.OutCubic
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

    // Um dado do campo: legenda pequena em cima e o valor embaixo.
    component Field: ColumnLayout {
        property string caption
        property string value
        property bool mono: true

        spacing: 0

        Label {
            text: parent.caption
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
        }

        Label {
            text: parent.value
            font.family: parent.mono ? Theme.monoFont : font.family
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
        anchors.leftMargin: Theme.pagePadding + 2
        spacing: Theme.spacingSm

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMd

            Row {
                spacing: Theme.spacingXs

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
                color: Theme.textSecondary
                elide: Text.ElideLeft
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }

            Item {
                visible: root.info.path === undefined
                Layout.fillWidth: true
            }
        }

        RowLayout {
            visible: root.expanded && root.info.path !== undefined
            spacing: Theme.spacingXl
            Layout.fillWidth: true

            // O tipo vira um selo na cor da categoria dele.
            ColumnLayout {
                spacing: 0

                Label {
                    text: qsTr("Tipo")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                }

                Rectangle {
                    readonly property color tone: Theme.typeColor(root.info.type ?? "", Theme.text)

                    radius: Theme.radiusSmall
                    color: Qt.alpha(tone, 0.15)
                    implicitWidth: typeLabel.implicitWidth + 12
                    implicitHeight: typeLabel.implicitHeight + 2

                    Label {
                        id: typeLabel

                        anchors.centerIn: parent
                        text: root.info.type ?? ""
                        color: parent.tone
                        font.family: Theme.monoFont
                        font.bold: true
                    }
                }
            }

            Field {
                caption: qsTr("Offset")
                value: root.info.offset ?? ""
            }

            Field {
                caption: qsTr("Bytes")
                value: root.info.size ?? ""
            }

            Field {
                caption: qsTr("Exibição")
                value: root.info.format ?? ""
            }

            Item {
                Layout.fillWidth: true
            }
        }

        RowLayout {
            visible: root.expanded && root.info.path !== undefined
            spacing: Theme.spacingMd
            Layout.fillWidth: true

            Label {
                text: qsTr("Valor")
                color: Theme.textSecondary
            }

            BinaTextField {
                id: valueField

                readOnly: true
                text: root.info.value ?? ""
                font.family: Theme.monoFont
                implicitHeight: 28
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
            color: Theme.textSecondary
        }

        Item {
            Layout.fillHeight: true
        }
    }
}

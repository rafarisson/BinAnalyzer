import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Aparência (tema e cor de destaque), opções do documento (ordem dos bytes e alinhamento máximo dos
// campos) e a pasta da biblioteca de tipos.
BinaPopup {
    id: root

    required property AppController app

    // Posiciona o popup logo abaixo do botão, alinhado à direita dele e dentro da janela.
    function placeBelow() {
        const overlay = Overlay.overlay
        if (!anchorItem || !overlay)
            return

        const below = anchorItem.mapToItem(overlay, anchorItem.width, anchorItem.height)
        const limit = Theme.pagePadding

        x = Math.max(limit, Math.min(below.x - width, overlay.width - width - limit))
        y = Math.max(limit, Math.min(below.y, overlay.height - height - limit))
    }

    // O botão que abre o popup: o popup abre logo abaixo dele, alinhado à direita e sem passar das
    // bordas da janela.
    property Item anchorItem

    parent: Overlay.overlay
    padding: Theme.spacingXl
    margins: Theme.pagePadding
    modal: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onAboutToShow: root.placeBelow()

    contentItem: ColumnLayout {
        spacing: Theme.spacingLg

        Label {
            text: qsTr("Aparência")
            font.bold: true
        }

        SegmentedControl {
            stretch: true
            Layout.fillWidth: true

            SegmentedButton {
                text: qsTr("Sistema")
                glyph: Icons.monitor
                checked: Theme.mode === Theme.modeSystem
                onClicked: Theme.mode = Theme.modeSystem
            }

            SegmentedButton {
                text: qsTr("Claro")
                glyph: Icons.whiteBalanceSunny
                checked: Theme.mode === Theme.modeLight
                onClicked: Theme.mode = Theme.modeLight
            }

            SegmentedButton {
                text: qsTr("Escuro")
                glyph: Icons.weatherNight
                checked: Theme.mode === Theme.modeDark
                onClicked: Theme.mode = Theme.modeDark
            }
        }

        RowLayout {
            spacing: Theme.spacingMd

            Label {
                text: qsTr("Cor de destaque")
                color: Theme.textSecondary
                Layout.rightMargin: Theme.spacingXs
            }

            Repeater {
                model: Theme.accents

                delegate: Rectangle {
                    id: swatch

                    required property var modelData

                    readonly property bool current: Theme.accentId === modelData.id

                    implicitWidth: 22
                    implicitHeight: 22
                    radius: width / 2
                    color: Theme.dark ? modelData.dark : modelData.light
                    border.width: current ? 2 : 0
                    border.color: Theme.text

                    Icon {
                        anchors.centerIn: parent
                        visible: swatch.current
                        text: Icons.check
                        font.pixelSize: 14
                        color: Theme.accentText
                    }

                    HoverHandler {
                        id: swatchHover

                        cursorShape: Qt.PointingHandCursor
                    }

                    TapHandler {
                        onTapped: Theme.accentId = swatch.modelData.id
                    }

                    ToolTip.visible: swatchHover.hovered
                    ToolTip.delay: 500
                    ToolTip.text: swatch.modelData.name
                }
            }
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        BinaSwitch {
            text: qsTr("Dados em big endian")
            checked: root.app.bigEndian
            onToggled: root.app.bigEndian = checked
        }

        RowLayout {
            spacing: 8

            Label {
                text: qsTr("Alinhamento máximo (bytes)")
            }

            BinaComboBox {
                model: [1, 2, 4, 8, 16]
                currentIndex: model.indexOf(root.app.maxAlign)
                onActivated: root.app.maxAlign = Number(currentText)
            }
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Biblioteca de tipos")
            font.bold: true
        }

        Label {
            text: root.app.typeLibraryPath
            color: Theme.textSecondary
            elide: Text.ElideMiddle
            Layout.maximumWidth: 360
        }

        RowLayout {
            spacing: 8

            BinaButton {
                text: qsTr("Abrir pasta")
                enabled: root.app.typeLibraryPath !== ""
                onClicked: root.app.openTypeLibraryFolder()
            }

            BinaButton {
                text: qsTr("Restaurar tipos padrão")
                enabled: root.app.typeLibraryPath !== ""
                onClicked: root.app.restoreDefaultTypes()

                ToolTip.visible: hovered
                ToolTip.delay: 500
                ToolTip.text: qsTr("Copia de novo os tipos que vêm com o programa, substituindo os de mesmo nome.")
            }
        }
    }
}

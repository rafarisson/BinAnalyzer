import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Opções do documento (ordem dos bytes e alinhamento máximo dos campos) e a pasta da biblioteca de
// tipos.
Popup {
    id: root

    required property AppController app

    padding: 12

    contentItem: ColumnLayout {
        spacing: 10

        Switch {
            text: qsTr("Dados em big endian")
            checked: root.app.bigEndian
            onToggled: root.app.bigEndian = checked
        }

        RowLayout {
            spacing: 8

            Label {
                text: qsTr("Alinhamento máximo (bytes)")
            }

            ComboBox {
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
            color: palette.placeholderText
            elide: Text.ElideMiddle
            Layout.maximumWidth: 360
        }

        RowLayout {
            spacing: 8

            Button {
                text: qsTr("Abrir pasta")
                enabled: root.app.typeLibraryPath !== ""
                onClicked: root.app.openTypeLibraryFolder()
            }

            Button {
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

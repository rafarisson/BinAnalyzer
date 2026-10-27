import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Opções do documento: ordem dos bytes e alinhamento máximo dos campos.
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
    }
}

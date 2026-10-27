import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// As definições do documento, uma visão por aba: os tipos customizados e os formatos customizados.
ColumnLayout {
    id: root

    required property AppController app

    function expandAll() {
        typePane.expandAll()
    }

    spacing: 0

    TabBar {
        id: tabs

        Layout.fillWidth: true

        TabButton {
            text: qsTr("Tipos")
        }

        TabButton {
            text: qsTr("Formatos")
        }
    }

    StackLayout {
        currentIndex: tabs.currentIndex
        Layout.fillWidth: true
        Layout.fillHeight: true

        TypePane {
            id: typePane

            typeController: root.app.typeController
        }

        FormatPane {
            formatController: root.app.formatController
        }
    }
}

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Painel da estrutura: os campos do documento e, abaixo, os tipos e os formatos customizados.
ColumnLayout {
    id: root

    required property AppController app

    // A estrutura e a árvore do tipo customizado.
    function expandAll() {
        view.expandAll()
        definitions.expandAll()
    }

    spacing: 0

    Label {
        text: qsTr("Estrutura")
        font.bold: true
        Layout.margins: Theme.pagePadding
    }

    SplitView {
        orientation: Qt.Vertical
        Layout.fillWidth: true
        Layout.fillHeight: true

        SchemaTreeView {
            id: view

            model: root.app.schemaModel
            SplitView.fillHeight: true
            SplitView.minimumHeight: 120
        }

        DefinitionsPane {
            id: definitions

            app: root.app

            SplitView.preferredHeight: 260
            SplitView.minimumHeight: 120
        }
    }
}

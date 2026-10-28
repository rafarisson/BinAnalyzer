import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Painel dos dados analisados, em árvore ou em tabela.
ColumnLayout {
    id: root

    required property AppController app

    signal openDataRequested()

    function expandAll() {
        treeView.expandAll()
    }

    function collapseAll() {
        treeView.collapseAll()
    }

    spacing: 0

    PaneHeader {
        title: qsTr("Dados")
        badge: root.app.session.recordCount === 0 ? "" : Number(root.app.session.recordCount).toLocaleString(Qt.locale(), "f", 0)
        Layout.fillWidth: true

        TreeActions {
            visible: !switcher.tableMode
            onExpandRequested: root.expandAll()
            onCollapseRequested: root.collapseAll()
        }

        IconButton {
            glyph: Icons.restore
            text: qsTr("Exibição padrão")
            visible: !switcher.tableMode
            implicitHeight: 26
            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: qsTr("Desfaz as trocas de formato feitas nos dados")
            onClicked: root.app.resultModel.resetFormats()
        }

        ViewSwitcher {
            id: switcher
        }
    }

    StaleBanner {
        visible: root.app.session.stale
        Layout.fillWidth: true
        onReanalyzeRequested: root.app.reanalyze()
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        StackLayout {
            anchors.fill: parent
            currentIndex: switcher.tableMode ? 1 : 0

            ResultTreeView {
                id: treeView

                model: root.app.resultModel
                session: root.app.session
            }

            ResultTableView {
                model: root.app.tableModel
            }
        }

        EmptyState {
            visible: !root.app.session.hasData
            anchors.fill: parent
            title: qsTr("Nenhum arquivo analisado")
            hint: qsTr("Abra um arquivo binário ou arraste-o para esta janela. Ele é analisado com a estrutura ao lado.")
            actionText: qsTr("Abrir binário…")
            onActionRequested: root.openDataRequested()
        }
    }
}

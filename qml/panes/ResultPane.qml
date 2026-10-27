import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Painel dos dados analisados, em árvore ou em tabela.
ColumnLayout {
    id: root

    required property AppController app

    function expandAll() {
        treeView.expandAll()
    }

    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: Theme.pagePadding

        Label {
            text: root.app.session.recordCount === 0
                  ? qsTr("Dados")
                  : root.app.session.recordCount === 1
                    ? qsTr("Dados — 1 registro")
                    : qsTr("Dados — %1 registros").arg(root.app.session.recordCount)
            font.bold: true
            Layout.fillWidth: true
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

    StackLayout {
        currentIndex: switcher.tableMode ? 1 : 0
        Layout.fillWidth: true
        Layout.fillHeight: true

        ResultTreeView {
            id: treeView

            model: root.app.resultModel
            session: root.app.session
        }

        ResultTableView {
            model: root.app.tableModel
        }
    }
}

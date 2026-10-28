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

        SplitView {
            anchors.fill: parent
            orientation: Qt.Vertical

            // O cabeçalho do arquivo (só se a análise o incluiu), no mesmo modo de exibição dos dados.
            ColumnLayout {
                visible: root.app.session.hasHeader
                spacing: 0
                SplitView.preferredHeight: 200
                SplitView.minimumHeight: Theme.headerHeight + 60

                PaneHeader {
                    title: qsTr("Cabeçalho")
                    badge: qsTr("%1 bytes").arg(root.app.session.headerSize)
                    Layout.fillWidth: true

                    TreeActions {
                        visible: !switcher.tableMode
                        onExpandRequested: headerTree.expandAll()
                        onCollapseRequested: headerTree.collapseAll()
                    }
                }

                StackLayout {
                    currentIndex: switcher.tableMode ? 1 : 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ResultTreeView {
                        id: headerTree

                        header: true
                        model: root.app.headerModel
                        session: root.app.session
                    }

                    ResultTableView {
                        model: root.app.headerTableModel
                    }
                }
            }

            StackLayout {
                currentIndex: switcher.tableMode ? 1 : 0
                SplitView.fillHeight: true
                SplitView.minimumHeight: 120

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

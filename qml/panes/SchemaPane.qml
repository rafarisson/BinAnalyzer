import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Painel da estrutura, uma visão por aba: os campos do documento, o cabeçalho do arquivo, os tipos
// customizados e os formatos customizados.
ColumnLayout {
    id: root

    required property AppController app

    readonly property int headerTab: 1
    readonly property int typesTab: 2
    readonly property int formatsTab: 3

    // Expandir e recolher valem para a árvore da aba aberta.
    function expandAll() {
        if (tabs.currentIndex === typesTab)
            typePane.expandAll()
        else if (tabs.currentIndex === headerTab)
            headerPane.expandAll()
        else if (tabs.currentIndex !== formatsTab)
            view.expandAll()
    }

    function collapseAll() {
        if (tabs.currentIndex === typesTab)
            typePane.collapseAll()
        else if (tabs.currentIndex === headerTab)
            headerPane.collapseAll()
        else if (tabs.currentIndex !== formatsTab)
            view.collapseAll()
    }

    spacing: 0

    Rectangle {
        color: Theme.headerBackground
        implicitHeight: Theme.headerHeight
        Layout.fillWidth: true

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.headerBorder
        }

        RowLayout {
            anchors.fill: parent
            anchors.rightMargin: Theme.pagePadding
            spacing: Theme.pagePadding

            TabBar {
                id: tabs

                background: null
                Layout.fillHeight: true

                component HeaderTab: TabButton {
                    id: tab

                    implicitHeight: Theme.headerHeight
                    contentItem: Label {
                        text: tab.text
                        font.bold: tab.checked
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: "transparent"

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 2
                            color: Theme.accent
                            visible: tab.checked
                        }
                    }
                }

                HeaderTab {
                    text: qsTr("Estrutura")
                }

                HeaderTab {
                    text: qsTr("Cabeçalho")
                }

                HeaderTab {
                    text: qsTr("Tipos")
                }

                HeaderTab {
                    text: qsTr("Formatos")
                }
            }

            Item {
                Layout.fillWidth: true
            }

            TreeActions {
                visible: tabs.currentIndex !== root.formatsTab
                onExpandRequested: root.expandAll()
                onCollapseRequested: root.collapseAll()
            }
        }
    }

    StackLayout {
        currentIndex: tabs.currentIndex
        Layout.fillWidth: true
        Layout.fillHeight: true

        SchemaTreeView {
            id: view

            model: root.app.schemaModel
        }

        TypePane {
            id: headerPane

            app: root.app
            typeController: root.app.headerController
        }

        TypePane {
            id: typePane

            app: root.app
            typeController: root.app.typeController
        }

        FormatPane {
            formatController: root.app.formatController
        }
    }
}

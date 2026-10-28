import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Barra de ferramentas: arquivo, análise, opções e mensagens. Os comandos que pedem um diálogo de
// arquivo são sinais; quem hospeda a barra abre o diálogo. Os atalhos valem na janela inteira.
ToolBar {
    id: root

    required property AppController app

    signal newSchemaRequested()
    signal openSchemaRequested()
    signal saveSchemaRequested()
    signal saveSchemaAsRequested()
    signal openDataRequested()
    signal exportResultRequested()

    Action {
        id: newAction

        text: qsTr("Nova estrutura")
        shortcut: "Ctrl+N"
        onTriggered: root.newSchemaRequested()
    }

    Action {
        id: openSchemaAction

        text: qsTr("Abrir estrutura…")
        shortcut: "Ctrl+O"
        onTriggered: root.openSchemaRequested()
    }

    Action {
        id: saveAction

        text: qsTr("Salvar")
        shortcut: "Ctrl+S"
        enabled: root.app.dirty || root.app.schemaPath === ""
        onTriggered: root.saveSchemaRequested()
    }

    Action {
        id: saveAsAction

        text: qsTr("Salvar como…")
        shortcut: "Ctrl+Shift+S"
        onTriggered: root.saveSchemaAsRequested()
    }

    Action {
        id: openDataAction

        text: qsTr("Abrir binário…")
        shortcut: "Ctrl+Shift+O"
        onTriggered: root.openDataRequested()
    }

    Action {
        id: reanalyzeAction

        text: qsTr("Analisar")
        shortcut: "F5"
        enabled: root.app.session.hasData
        onTriggered: root.app.reanalyze()
    }

    Action {
        id: exportAction

        text: qsTr("Exportar…")
        shortcut: "Ctrl+E"
        enabled: root.app.session.hasData
        onTriggered: root.exportResultRequested()
    }

    // Botão de ação com a dica "nome (atalho)".
    component BarButton: IconButton {
        id: button

        property string hint

        ToolTip.visible: hovered
        ToolTip.delay: 500
        ToolTip.text: action && action.shortcut ? "%1 (%2)".arg(hint || action.text).arg(action.shortcut) : hint
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: 4

        BarButton {
            id: fileButton

            glyph: Icons.menu
            text: qsTr("Arquivo")
            onClicked: fileMenu.open()

            Menu {
                id: fileMenu

                y: fileButton.height

                MenuItem {
                    action: newAction
                }
                MenuItem {
                    action: openSchemaAction
                }
                MenuSeparator {}
                MenuItem {
                    action: saveAction
                }
                MenuItem {
                    action: saveAsAction
                }
            }
        }

        BarButton {
            action: saveAction
            glyph: Icons.contentSave
            text: qsTr("Salvar")
        }

        ToolSeparator {}

        BarButton {
            action: openDataAction
            glyph: Icons.folderOpenOutline
        }

        BarButton {
            id: analyzeButton

            readonly property bool pending: root.app.session.stale

            action: reanalyzeAction
            glyph: Icons.play
            text: qsTr("Analisar")
            hint: qsTr("Analisa o arquivo com a estrutura atual")
            font.bold: pending
            contentColor: pending && enabled ? Theme.accentText : palette.buttonText

            // Destaca o botão quando a estrutura mudou depois da última análise.
            background: Rectangle {
                radius: 4
                color: analyzeButton.pending && analyzeButton.enabled ? Theme.accent
                       : analyzeButton.down ? analyzeButton.palette.mid
                       : analyzeButton.hovered ? analyzeButton.palette.midlight
                       : "transparent"
            }
        }

        BarButton {
            action: exportAction
            glyph: Icons.fileExportOutline
            hint: qsTr("Exporta o resultado como árvore (JSON) ou tabela (CSV)")
        }

        Item {
            Layout.fillWidth: true
        }

        BarButton {
            id: optionsButton

            glyph: Icons.cog
            text: qsTr("Opções")
            onClicked: options.open()

            OptionsPopup {
                id: options

                app: root.app
                anchorItem: optionsButton
            }
        }

        BarButton {
            id: messagesButton

            glyph: Icons.bellOutline
            text: root.app.messageLog.count > 0 ? qsTr("Mensagens (%1)").arg(root.app.messageLog.count) : qsTr("Mensagens")
            onClicked: messages.open()

            MessagesPopup {
                id: messages

                app: root.app
                anchorItem: messagesButton
            }
        }
    }
}

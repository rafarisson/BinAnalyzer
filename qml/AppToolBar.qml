import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Barra de ferramentas: comandos de arquivo, análise, opções e mensagens. Os comandos que pedem um
// diálogo de arquivo são sinais; quem hospeda a barra abre o diálogo.
ToolBar {
    id: root

    required property AppController app

    signal newSchemaRequested()
    signal openSchemaRequested()
    signal saveSchemaRequested()
    signal saveSchemaAsRequested()
    signal openDataRequested()
    signal exportResultRequested()
    signal expandAllRequested()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: 4

        ToolButton {
            text: qsTr("Nova")
            onClicked: root.newSchemaRequested()
        }
        ToolButton {
            text: qsTr("Abrir estrutura…")
            onClicked: root.openSchemaRequested()
        }
        ToolButton {
            text: qsTr("Salvar")
            enabled: root.app.dirty || root.app.schemaPath === ""
            onClicked: root.saveSchemaRequested()
        }
        ToolButton {
            text: qsTr("Salvar como…")
            onClicked: root.saveSchemaAsRequested()
        }

        ToolSeparator {}

        ToolButton {
            text: qsTr("Abrir binário…")
            onClicked: root.openDataRequested()
        }
        ToolButton {
            text: qsTr("Analisar novamente")
            enabled: root.app.session.hasData
            font.bold: root.app.session.stale
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Analisa o arquivo com a estrutura atual")
            onClicked: root.app.reanalyze()
        }
        ToolButton {
            text: qsTr("Exportar…")
            enabled: root.app.session.hasData
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Exporta o resultado como árvore (JSON) ou tabela (CSV)")
            onClicked: root.exportResultRequested()
        }
        ToolButton {
            text: qsTr("Exibição padrão")
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Desfaz as trocas de formato feitas nos dados")
            onClicked: root.app.resultModel.resetFormats()
        }
        ToolButton {
            text: qsTr("Expandir tudo")
            onClicked: root.expandAllRequested()
        }

        ToolSeparator {}

        ToolButton {
            id: optionsButton

            text: qsTr("Opções")
            onClicked: options.open()

            OptionsPopup {
                id: options

                app: root.app
                y: optionsButton.height
            }
        }
        ToolButton {
            id: messagesButton

            text: root.app.messageLog.count > 0 ? qsTr("Mensagens (%1)").arg(root.app.messageLog.count) : qsTr("Mensagens")
            onClicked: messages.open()

            MessagesPopup {
                id: messages

                app: root.app
                y: messagesButton.height
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: root.app.session.statusText
            elide: Text.ElideLeft
            Layout.maximumWidth: 420
        }
    }
}

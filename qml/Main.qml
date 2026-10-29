import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls.Basic

import Bina.App

ApplicationWindow {
    id: window

    required property AppController app

    // Evita perguntar de novo ao fechar depois de a pessoa decidir descartar as alterações.
    property bool closeConfirmed: false

    // "Salvar" na confirmação de fechamento, sem caminho: fecha depois que o "Salvar como" der certo.
    property bool closeAfterSave: false

    function closeNow() {
        closeConfirmed = true
        close()
    }

    width: 1400
    height: 760
    minimumWidth: 700
    minimumHeight: 300
    visible: true
    title: app.windowTitle
    color: Theme.window

    // A paleta vem do tema (e não do sistema): todos os controles a herdam, inclusive popups e menus.
    palette {
        window: Theme.window
        windowText: Theme.text
        base: Theme.surface
        alternateBase: Theme.surfaceAlt
        text: Theme.text
        button: Theme.control
        buttonText: Theme.text
        highlight: Theme.accent
        highlightedText: Theme.accentText
        placeholderText: Theme.textSecondary
        toolTipBase: Theme.raised
        toolTipText: Theme.text
        light: Theme.controlHover
        midlight: Theme.controlHover
        mid: Theme.border
        dark: Theme.borderStrong
        shadow: Theme.shadow
        link: Theme.accent
    }

    function saveSchema() {
        if (app.schemaPath === "")
            saveSchemaDialog.open()
        else
            app.saveSchema()
    }

    onClosing: close => {
        if (app.dirty && !closeConfirmed) {
            close.accepted = false
            confirmClose.open()
        }
    }

    header: AppToolBar {
        app: window.app

        onNewSchemaRequested: window.app.newSchema()
        onOpenSchemaRequested: openSchemaDialog.open()
        onSaveSchemaRequested: window.saveSchema()
        onSaveSchemaAsRequested: saveSchemaDialog.open()
        onOpenDataRequested: openDataDialog.open()
        onExportResultRequested: exportResultDialog.open()
    }

    footer: AppStatusBar {
        app: window.app
    }

    FileDialog {
        id: openSchemaDialog

        title: qsTr("Abrir estrutura")
        nameFilters: [qsTr("Estrutura (*.json)"), qsTr("Todos os arquivos (*)")]
        onAccepted: window.app.openSchema(selectedFile.toString())
    }

    FileDialog {
        id: saveSchemaDialog

        title: qsTr("Salvar estrutura")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Estrutura (*.json)")]
        onAccepted: {
            const saved = window.app.saveSchemaAs(selectedFile.toString())

            if (saved && window.closeAfterSave)
                window.closeNow()

            window.closeAfterSave = false
        }
        onRejected: window.closeAfterSave = false
    }

    FileDialog {
        id: openDataDialog

        title: qsTr("Abrir arquivo binário")
        onAccepted: window.app.openData(selectedFile.toString())
    }

    FileDialog {
        id: exportResultDialog

        title: qsTr("Exportar resultado")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("JSON - árvore (*.json)"), qsTr("CSV - tabela (*.csv)")]
        onAccepted: window.app.exportResult(selectedFile.toString())
    }

    BinaDialog {
        id: confirmClose

        title: qsTr("Alterações não salvas")
        anchors.centerIn: Overlay.overlay
        width: 420
        modal: true
        standardButtons: Dialog.Save | Dialog.Discard | Dialog.Cancel

        contentItem: Label {
            text: qsTr("A estrutura tem alterações não salvas. Salvar antes de fechar?")
            wrapMode: Text.WordWrap
        }

        // Se o salvamento falhar, a janela fica aberta e a mensagem de erro aparece.
        onAccepted: {
            if (window.app.schemaPath === "") {
                window.closeAfterSave = true
                saveSchemaDialog.open()
            } else if (window.app.saveSchema()) {
                window.closeNow()
            }
        }

        onDiscarded: window.closeNow()
    }

    SplitView {
        anchors.fill: parent

        handle: Rectangle {
            implicitWidth: 5
            implicitHeight: 5
            color: SplitHandle.pressed ? Theme.accent
                   : SplitHandle.hovered ? Qt.alpha(Theme.accent, 0.6)
                   : Theme.border
            opacity: SplitHandle.pressed || SplitHandle.hovered ? 1 : 0.6

            Behavior on color {
                ColorAnimation {
                    duration: Theme.animFast
                }
            }
        }

        SchemaPane {
            id: schemaPane

            app: window.app
            SplitView.preferredWidth: 560
            SplitView.minimumWidth: 300
        }

        ResultPane {
            id: resultPane

            app: window.app
            onOpenDataRequested: openDataDialog.open()
            SplitView.fillWidth: true
            SplitView.minimumWidth: 300
        }
    }

    // Soltar um arquivo na janela: .json abre como estrutura, qualquer outro como binário.
    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]

        onDropped: drop => {
            if (drop.urls.length === 0)
                return

            const url = drop.urls[0].toString()

            if (url.toLowerCase().endsWith(".json"))
                window.app.openSchema(url)
            else
                window.app.openData(url)
        }

        Rectangle {
            anchors.fill: parent
            visible: parent.containsDrag
            color: "transparent"
            border.color: Theme.accent
            border.width: 3
            opacity: 0.8
        }
    }

    StatusToast {
        id: toast

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        z: 100
    }

    Connections {
        target: window.app

        function onMessageAdded(text, isError) {
            toast.show(text, isError)
        }
    }
}

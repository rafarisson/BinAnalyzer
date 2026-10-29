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

    // A janela só aparece depois de restaurar a posição e o tamanho (ver Component.onCompleted).
    width: 1400
    height: 760
    minimumWidth: 700
    minimumHeight: 300
    visible: false
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

    // A área gravada ainda aparece em algum monitor? (Um monitor pode ter sido desconectado.)
    function fitsOnAScreen(rect) {
        for (const screen of Qt.application.screens) {
            const width = Math.min(rect.x + rect.width, screen.virtualX + screen.width) - Math.max(rect.x, screen.virtualX)
            const height = Math.min(rect.y + rect.height, screen.virtualY + screen.height) - Math.max(rect.y, screen.virtualY)

            // A barra de título precisa estar à vista para a janela poder ser movida.
            if (width >= 120 && height >= 60 && rect.y >= screen.virtualY && rect.y < screen.virtualY + screen.height - 40)
                return true
        }
        return false
    }

    function restoreWindow() {
        const rect = AppSettings.windowRect

        if (rect.width > 0 && window.fitsOnAScreen(rect)) {
            window.x = rect.x
            window.y = rect.y
            window.width = Math.max(window.minimumWidth, rect.width)
            window.height = Math.max(window.minimumHeight, rect.height)
        }

        window.visible = true

        if (AppSettings.windowMaximized)
            window.showMaximized()
    }

    // Guarda a janela (só a área normal: maximizada, vale a de antes) e a largura da estrutura.
    function saveWindow() {
        if (window.visibility === Window.Windowed) {
            AppSettings.windowRect = Qt.rect(window.x, window.y, window.width, window.height)
            AppSettings.windowMaximized = false
        } else if (window.visibility === Window.Maximized) {
            AppSettings.windowMaximized = true
        }

        if (schemaPane.width > 0)
            AppSettings.schemaPaneWidth = Math.round(schemaPane.width)

        AppSettings.sync()
    }

    Component.onCompleted: restoreWindow()

    function saveSchema() {
        if (app.schemaPath === "")
            saveSchemaDialog.open()
        else
            app.saveSchema()
    }

    onClosing: close => {
        window.saveWindow()

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
        onAccepted: {
            AppSettings.rememberFolder("schema", selectedFile)
            window.app.openSchema(selectedFile.toString())
        }
    }

    FileDialog {
        id: saveSchemaDialog

        title: qsTr("Salvar estrutura")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Estrutura (*.json)")]
        onAccepted: {
            AppSettings.rememberFolder("schema", selectedFile)
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
        onAccepted: {
            AppSettings.rememberFolder("data", selectedFile)
            window.app.openData(selectedFile.toString())
        }
    }

    FileDialog {
        id: exportResultDialog

        title: qsTr("Exportar resultado")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("JSON - árvore (*.json)"), qsTr("CSV - tabela (*.csv)")]
        onAccepted: {
            AppSettings.rememberFolder("export", selectedFile)
            window.app.exportResult(selectedFile.toString())
        }
    }

    // Cada diálogo abre na última pasta usada nele. A ligação (e não uma atribuição) continua valendo
    // depois de o diálogo navegar por conta própria.
    Binding {
        target: openSchemaDialog
        property: "currentFolder"
        value: AppSettings.schemaFolder
    }

    Binding {
        target: saveSchemaDialog
        property: "currentFolder"
        value: AppSettings.schemaFolder
    }

    Binding {
        target: openDataDialog
        property: "currentFolder"
        value: AppSettings.dataFolder
    }

    Binding {
        target: exportResultDialog
        property: "currentFolder"
        value: AppSettings.exportFolder
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
            SplitView.preferredWidth: AppSettings.schemaPaneWidth > 0 ? AppSettings.schemaPaneWidth : 560
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

        // Os arquivos abertos (ou salvos) entram nos recentes, venham de onde vierem: diálogo,
        // soltar na janela, menu de recentes. Os exemplos embutidos não entram.
        function onSchemaPathChanged() {
            AppSettings.addRecentSchema(window.app.schemaPath)
        }

        function onDataPathChanged() {
            AppSettings.addRecentData(window.app.dataPath)
        }
    }
}

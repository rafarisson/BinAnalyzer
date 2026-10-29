import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Formatos customizados do documento: o ComboBox escolhe o formato e mostra o seu padrão. Abaixo,
// o teste: um padrão (começa com o do formato escolhido, mas pode ser qualquer um) e uma amostra de
// valores mostram o texto que sairia. O teste não altera o documento.
ColumnLayout {
    id: root

    required property FormatPatternController formatController

    readonly property bool hasFormats: formatController.patternIds.length > 0
    readonly property var outcome: formatController.preview(patternField.text, sampleField.text)

    // O padrão digitado é válido, diferente do aplicado e há um formato para receber a mudança.
    readonly property bool canApply: hasFormats && outcome.patternValid
                                     && patternField.text !== formatController.currentSpec

    // Escolher outro formato leva o padrão dele e uma amostra para o teste.
    function loadCurrent() {
        patternField.text = formatController.currentSpec
        sampleField.text = formatController.suggestSample(patternField.text)
    }

    spacing: Theme.pagePadding

    Component.onCompleted: loadCurrent()

    Connections {
        target: root.formatController

        function onCurrentPatternChanged() {
            root.loadCurrent()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: Theme.pagePadding

        BinaComboBox {
            id: combo

            model: root.formatController.patternIds
            enabled: root.hasFormats
            displayText: root.hasFormats ? currentText : qsTr("(nenhum formato definido)")
            Layout.fillWidth: true

            onActivated: root.formatController.currentPattern = currentText

            // O ComboBox troca o próprio currentIndex ao escolher; a ligação o mantém igual ao model.
            Binding {
                target: combo
                property: "currentIndex"
                value: root.formatController.patternIds.indexOf(root.formatController.currentPattern)
            }
        }

        BinaButton {
            text: qsTr("Novo...")
            onClicked: newFormat.open()
        }
    }

    NewItemPopup {
        id: newFormat

        title: qsTr("Novo formato")
        askPattern: true
        onAccepted: (name, pattern) => root.formatController.createPattern(name, pattern)
    }

    GridLayout {
        columns: 2
        columnSpacing: Theme.cellPadding
        rowSpacing: Theme.pagePadding
        Layout.fillWidth: true
        Layout.leftMargin: Theme.pagePadding
        Layout.rightMargin: Theme.pagePadding

        Label {
            text: qsTr("Padrão")
        }

        BinaTextField {
            id: patternField

            placeholderText: qsTr("%(campo)d, %x, %s ...")
            font.family: Theme.monoFont
            selectByMouse: true
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Amostra")
        }

        RowLayout {
            Layout.fillWidth: true

            BinaTextField {
                id: sampleField

                placeholderText: qsTr("campo=valor, outro=2.5  ou  1, 2, texto")
                font.family: Theme.monoFont
                selectByMouse: true
                Layout.fillWidth: true
            }

            BinaButton {
                text: qsTr("Sugerir")
                enabled: patternField.text !== ""
                onClicked: sampleField.text = root.formatController.suggestSample(patternField.text)
            }
        }

        // Célula vazia da primeira coluna do grid.
        Item {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 1
        }

        RowLayout {
            Layout.fillWidth: true

            BinaButton {
                text: qsTr("Aplicar")
                enabled: root.canApply
                highlighted: root.canApply
                onClicked: root.formatController.applyPattern(patternField.text)

                ToolTip.visible: hovered
                ToolTip.delay: 500
                ToolTip.text: qsTr("Usa este padrão no formato selecionado. Os dados precisam ser analisados novamente.")
            }

            BinaButton {
                text: qsTr("Restaurar")
                enabled: root.hasFormats && patternField.text !== root.formatController.currentSpec
                onClicked: patternField.text = root.formatController.currentSpec
            }

            Label {
                visible: root.hasFormats && patternField.text !== root.formatController.currentSpec
                text: qsTr("Alterações ainda não aplicadas")
                color: Theme.textSecondary
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Resultado")
        }

        Rectangle {
            color: root.outcome.ok ? Theme.surface : Theme.errorBackground
            border.color: Theme.border
            implicitHeight: resultLabel.implicitHeight + 2 * Theme.cellPadding
            Layout.fillWidth: true

            Label {
                id: resultLabel

                anchors.fill: parent
                anchors.margins: Theme.cellPadding
                text: root.outcome.ok ? root.outcome.text : root.outcome.error
                color: root.outcome.ok ? Theme.text : Theme.errorText
                font.family: Theme.monoFont
                wrapMode: Text.WrapAnywhere
                elide: Text.ElideNone
            }
        }
    }

    Label {
        text: qsTr("Campos por nome: %(campo)d, com caminho %(grupo.campo)d e ajuste %(ano+1900)d. "
                   + "Sem nome, os valores da amostra são usados em ordem.")
        color: Theme.textSecondary
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        Layout.leftMargin: Theme.pagePadding
        Layout.rightMargin: Theme.pagePadding
    }

    Item {
        Layout.fillHeight: true
    }
}

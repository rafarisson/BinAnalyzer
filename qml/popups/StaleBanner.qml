import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Aviso de que a estrutura, os tipos ou os formatos mudaram depois da última análise.
Rectangle {
    id: root

    signal reanalyzeRequested()

    color: Theme.warningBackground
    border.color: Theme.warningBorder
    implicitHeight: row.implicitHeight + 2 * Theme.pagePadding

    RowLayout {
        id: row

        anchors.fill: parent
        anchors.margins: Theme.pagePadding

        Label {
            text: qsTr("A estrutura, os tipos ou os formatos foram alterados: os dados abaixo estão desatualizados. Analise o arquivo novamente.")
            color: Theme.warningText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Button {
            text: qsTr("Analisar novamente")
            onClicked: root.reanalyzeRequested()
        }
    }
}

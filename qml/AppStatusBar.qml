import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Bina.App

// Barra de status: resultado da última análise, arquivo binário, estrutura e alterações pendentes.
Rectangle {
    id: root

    required property AppController app

    function fileName(path) {
        const parts = String(path).split(/[\\/]/)
        return parts[parts.length - 1]
    }

    color: Theme.headerBackground
    implicitHeight: 26

    Rectangle {
        width: parent.width
        height: 1
        color: Theme.headerBorder
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding + 2
        anchors.rightMargin: Theme.pagePadding + 2
        spacing: Theme.pagePadding * 2

        Label {
            text: root.app.session.statusText
            elide: Text.ElideRight
            color: Theme.textSecondary
            font.pixelSize: Theme.fontCaption
            Layout.fillWidth: true
        }

        Label {
            visible: root.app.dataPath !== ""
            text: qsTr("Binário: %1").arg(root.fileName(root.app.dataPath))
            elide: Text.ElideMiddle
            color: Theme.textSecondary
            font.pixelSize: Theme.fontCaption
            Layout.maximumWidth: 300
        }

        Label {
            text: qsTr("Estrutura: %1").arg(root.app.schemaPath === "" ? qsTr("Sem título") : root.fileName(root.app.schemaPath))
            elide: Text.ElideMiddle
            color: Theme.textSecondary
            font.pixelSize: Theme.fontCaption
            Layout.maximumWidth: 300
        }

        Label {
            visible: root.app.dirty
            text: qsTr("● não salvo")
            color: Theme.warningText
            font.pixelSize: Theme.fontCaption
            font.bold: true
        }
    }
}

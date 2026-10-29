import QtQuick
import QtQuick.Controls.Basic

// Título de uma coluna de tabela ou árvore (delegate dos HorizontalHeaderView).
Rectangle {
    id: root

    required property var model

    implicitWidth: label.implicitWidth + 2 * Theme.cellPadding
    implicitHeight: Theme.rowHeight
    color: Theme.headerBackground

    Label {
        id: label

        anchors.fill: parent
        anchors.leftMargin: Theme.cellPadding
        anchors.rightMargin: Theme.cellPadding
        verticalAlignment: Text.AlignVCenter
        text: String(root.model.display ?? "")
        elide: Text.ElideRight
        font.bold: true
        font.pixelSize: Theme.fontCaption
        color: Theme.textSecondary
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.headerBorder
    }

    Rectangle {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 1
        height: parent.height * 0.5
        color: Theme.headerBorder
    }
}

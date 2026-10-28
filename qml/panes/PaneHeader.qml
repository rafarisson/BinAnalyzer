import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Cabeçalho de painel: título, selo com a contagem e, à direita, as ações do painel (os filhos).
Rectangle {
    id: root

    property string title
    property string badge

    default property alias actions: actionsRow.data

    color: Theme.headerBackground
    implicitHeight: Theme.headerHeight

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.headerBorder
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding + 2
        anchors.rightMargin: Theme.pagePadding
        spacing: Theme.pagePadding

        Label {
            text: root.title
            font.bold: true
        }

        Rectangle {
            visible: root.badge !== ""
            radius: height / 2
            color: Theme.badgeBackground
            implicitWidth: badgeLabel.implicitWidth + 12
            implicitHeight: badgeLabel.implicitHeight + 2

            Label {
                id: badgeLabel

                anchors.centerIn: parent
                text: root.badge
                font.pixelSize: 11
            }
        }

        Item {
            Layout.fillWidth: true
        }

        RowLayout {
            id: actionsRow

            spacing: 2
        }
    }
}

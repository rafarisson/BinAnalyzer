import QtQuick
import QtQuick.Controls.Basic

// Entrada de menu: marca de seleção à esquerda quando 'checkable' e atalho à direita.
MenuItem {
    id: root

    implicitHeight: Theme.controlHeight - 2
    implicitWidth: Math.max(160, implicitContentWidth + leftPadding + rightPadding)
    leftPadding: Theme.spacingMd
    rightPadding: Theme.spacingMd
    topPadding: 0
    bottomPadding: 0
    spacing: Theme.spacingSm

    indicator: Icon {
        x: root.leftPadding
        y: (root.height - height) / 2
        visible: root.checkable
        text: root.checked ? Icons.check : ""
        font.pixelSize: 16
        color: Theme.accent
    }

    arrow: null

    contentItem: Item {
        implicitWidth: label.implicitWidth + (shortcut.visible ? shortcut.implicitWidth + 24 : 0)
                       + (root.checkable ? 22 : 0)
        implicitHeight: label.implicitHeight

        Text {
            id: label

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: root.checkable ? 22 : 0
            text: root.text
            font: root.font
            color: root.enabled ? Theme.text : Theme.textDisabled
        }

        Text {
            id: shortcut

            visible: root.action !== null && root.action.shortcut !== undefined
                     && String(root.action.shortcut) !== ""
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            text: visible ? String(root.action.shortcut) : ""
            font.pixelSize: Theme.fontCaption
            color: Theme.textSecondary
        }
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: root.highlighted && root.enabled ? Theme.selectionFill : "transparent"
    }
}

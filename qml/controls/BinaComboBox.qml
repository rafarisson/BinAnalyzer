import QtQuick
import QtQuick.Controls.Basic

// Lista de escolha do aplicativo, com a lista suspensa no mesmo estilo dos menus.
ComboBox {
    id: root

    implicitHeight: Theme.controlHeight
    implicitWidth: Math.max(120, implicitContentWidth + leftPadding + rightPadding)
    leftPadding: Theme.spacingMd
    rightPadding: indicator.width + Theme.spacingMd
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true

    delegate: ItemDelegate {
        id: item

        width: ListView.view ? ListView.view.width : root.width
        height: Theme.controlHeight - 2
        leftPadding: Theme.spacingMd
        rightPadding: Theme.spacingMd
        highlighted: root.highlightedIndex === index
        text: itemText()

        // O texto vem do papel do model; em listas de objetos (JS ou list<var>) o objeto é o modelData.
        function itemText() {
            if (!root.textRole)
                return modelData
            const source = typeof modelData === "object" && modelData !== null ? modelData : model
            return source[root.textRole]
        }

        contentItem: Text {
            text: item.text
            font: root.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            color: root.currentIndex === index ? Theme.accent : Theme.text
        }

        background: Rectangle {
            radius: Theme.radiusSmall
            color: item.highlighted ? Theme.selectionFill : "transparent"
        }
    }

    indicator: Icon {
        x: root.width - width - Theme.spacingXs
        y: (root.height - height) / 2
        width: 24
        text: Icons.chevronDown
        font.pixelSize: 16
        color: root.enabled ? Theme.textSecondary : Theme.textDisabled
        rotation: root.popup.visible ? 180 : 0

        Behavior on rotation {
            NumberAnimation {
                duration: Theme.animFast
            }
        }
    }

    contentItem: Text {
        text: root.displayText
        font: root.font
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
        color: root.enabled ? Theme.text : Theme.textDisabled
    }

    background: Rectangle {
        radius: Theme.radiusMedium
        color: !root.enabled ? Theme.disabledFill
               : root.down ? Theme.controlPressed
               : root.hovered ? Theme.controlHover
               : Theme.control
        border.width: root.visualFocus || root.popup.visible ? 2 : 1
        border.color: root.visualFocus || root.popup.visible ? Theme.accent
                      : root.hovered ? Theme.borderStrong
                      : Theme.border

        Behavior on color {
            ColorAnimation {
                duration: Theme.animFast
            }
        }
    }

    popup: Popup {
        y: root.height + 4
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight + 2 * padding, 300)
        padding: Theme.spacingXs

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex

            ScrollBar.vertical: BinaScrollBar {}
        }

        background: BinaPopupBackground {
            radius: Theme.radiusMedium
        }

        enter: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: Theme.animFast
            }
        }
    }
}

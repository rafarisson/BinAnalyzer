import QtQuick
import QtQuick.Controls.Basic

// Interruptor do aplicativo.
Switch {
    id: root

    implicitHeight: Theme.controlHeight
    spacing: Theme.spacingMd
    hoverEnabled: true

    indicator: Rectangle {
        x: root.leftPadding
        y: (root.height - height) / 2
        implicitWidth: 36
        implicitHeight: 20
        radius: height / 2
        color: !root.enabled ? Theme.disabledFill
               : root.checked ? (root.hovered ? Theme.accentHover : Theme.accent)
               : root.hovered ? Theme.controlPressed : Theme.controlHover
        border.width: root.checked || !root.enabled ? 0 : 1
        border.color: root.visualFocus ? Theme.accent : Theme.borderStrong

        Behavior on color {
            ColorAnimation {
                duration: Theme.animFast
            }
        }

        Rectangle {
            x: root.checked ? parent.width - width - 3 : 3
            anchors.verticalCenter: parent.verticalCenter
            width: 14
            height: 14
            radius: 7
            color: root.checked ? Theme.accentText : root.enabled ? Theme.textSecondary : Theme.textDisabled

            Behavior on x {
                NumberAnimation {
                    duration: Theme.animFast
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    contentItem: Text {
        leftPadding: root.indicator.width + root.spacing
        text: root.text
        font: root.font
        verticalAlignment: Text.AlignVCenter
        color: root.enabled ? Theme.text : Theme.textDisabled
    }
}

import QtQuick
import QtQuick.Controls.Basic

// Botão do aplicativo. Três variações: padrão (secundário), 'highlighted' (primário, na cor de destaque)
// e 'flat' (discreto, sem fundo até a passagem do mouse).
Button {
    id: root

    readonly property bool primary: highlighted

    implicitHeight: Theme.controlHeight
    implicitWidth: Math.max(64, implicitContentWidth + leftPadding + rightPadding)
    leftPadding: Theme.spacingLg + 2
    rightPadding: Theme.spacingLg + 2
    topPadding: 0
    bottomPadding: 0
    spacing: Theme.spacingSm
    hoverEnabled: true

    contentItem: Text {
        text: root.text
        font: root.font
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: !root.enabled ? Theme.textDisabled
               : root.primary ? Theme.accentText
               : Theme.text
    }

    background: Rectangle {
        radius: Theme.radiusMedium
        color: {
            if (!root.enabled)
                return root.flat ? "transparent" : Theme.disabledFill
            if (root.primary)
                return root.down ? Theme.accentPressed : root.hovered ? Theme.accentHover : Theme.accent
            if (root.flat)
                return root.down ? Theme.pressedOverlay : root.hovered ? Theme.hoverOverlay : "transparent"
            return root.down ? Theme.controlPressed : root.hovered ? Theme.controlHover : Theme.control
        }
        border.width: root.flat || root.primary || !root.enabled ? 0 : 1
        border.color: root.visualFocus ? Theme.accent : Theme.borderStrong

        Behavior on color {
            ColorAnimation {
                duration: Theme.animFast
            }
        }

        // Anel de foco do teclado.
        Rectangle {
            visible: root.visualFocus
            anchors.fill: parent
            anchors.margins: -2
            radius: parent.radius + 2
            color: "transparent"
            border.width: 2
            border.color: Qt.alpha(Theme.accent, 0.6)
        }
    }
}

import QtQuick
import QtQuick.Controls.Basic

// Campo de texto do aplicativo. 'invalid' pinta a borda de erro.
TextField {
    id: root

    property bool invalid: false

    implicitHeight: Theme.controlHeight
    leftPadding: Theme.spacingMd
    rightPadding: Theme.spacingMd
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter
    selectByMouse: true
    hoverEnabled: true
    color: enabled ? Theme.text : Theme.textDisabled
    placeholderTextColor: Theme.textDisabled
    selectionColor: Qt.alpha(Theme.accent, 0.45)
    selectedTextColor: Theme.text

    background: Rectangle {
        radius: Theme.radiusMedium
        color: !root.enabled ? Theme.disabledFill : root.readOnly ? Theme.surfaceAlt : Theme.surface
        border.width: root.activeFocus ? 2 : 1
        border.color: root.invalid ? Theme.dropInvalid
                      : root.activeFocus ? Theme.accent
                      : root.hovered ? Theme.borderStrong
                      : Theme.border

        Behavior on border.color {
            ColorAnimation {
                duration: Theme.animFast
            }
        }
    }
}

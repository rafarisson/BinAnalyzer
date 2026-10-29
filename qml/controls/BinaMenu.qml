import QtQuick
import QtQuick.Controls.Basic

// Menu do aplicativo, no mesmo estilo dos popups.
Menu {
    padding: Theme.spacingXs
    overlap: 2

    delegate: BinaMenuItem {}

    background: BinaPopupBackground {
        implicitWidth: 180
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

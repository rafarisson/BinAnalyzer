import QtQuick
import QtQuick.Controls.Basic

// Popup do aplicativo: fundo elevado com sombra, fundo escurecido se modal e entrada suave.
Popup {
    id: root

    padding: Theme.spacingXl

    background: BinaPopupBackground {}

    Overlay.modal: Rectangle {
        color: Theme.scrim
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Theme.animNormal
        }
        NumberAnimation {
            property: "scale"
            from: 0.97
            to: 1
            duration: Theme.animNormal
            easing.type: Easing.OutCubic
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: Theme.animFast
        }
    }
}

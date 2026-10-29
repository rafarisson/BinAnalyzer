import QtQuick
import QtQuick.Controls.Basic

// Diálogo padrão do aplicativo: o botão de aceitar fica na cor de destaque.
Dialog {
    id: root

    padding: Theme.spacingXl
    topPadding: Theme.spacingMd
    bottomPadding: Theme.spacingXl

    background: BinaPopupBackground {}

    Overlay.modal: Rectangle {
        color: Theme.scrim
    }

    header: Text {
        visible: root.title !== ""
        text: root.title
        font.bold: true
        font.pixelSize: Theme.fontTitle
        color: Theme.text
        elide: Text.ElideRight
        leftPadding: Theme.spacingXl
        rightPadding: Theme.spacingXl
        topPadding: Theme.spacingXl
        bottomPadding: Theme.spacingXs
    }

    footer: DialogButtonBox {
        spacing: Theme.spacingMd
        padding: Theme.spacingXl
        topPadding: 0
        background: null
        alignment: Qt.AlignRight

        delegate: BinaButton {
            highlighted: DialogButtonBox.buttonRole === DialogButtonBox.AcceptRole
                         || DialogButtonBox.buttonRole === DialogButtonBox.YesRole
        }
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

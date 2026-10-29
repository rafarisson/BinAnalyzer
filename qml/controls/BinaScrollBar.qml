import QtQuick
import QtQuick.Controls.Basic

// Barra de rolagem fina que só aparece quando a lista rola ou o mouse chega perto.
ScrollBar {
    id: root

    padding: 2
    minimumSize: 0.08
    hoverEnabled: true

    contentItem: Rectangle {
        implicitWidth: root.hovered || root.pressed ? 10 : 7
        implicitHeight: root.hovered || root.pressed ? 10 : 7
        radius: Math.min(width, height) / 2
        color: root.pressed ? Qt.alpha(Theme.text, 0.55) : Qt.alpha(Theme.text, root.hovered ? 0.4 : 0.28)
        opacity: root.policy === ScrollBar.AlwaysOn || (root.active && root.size < 1.0) ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.animNormal
            }
        }
    }

    background: null
}

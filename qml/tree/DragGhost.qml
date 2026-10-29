import QtQuick
import QtQuick.Controls.Basic

// O rótulo que acompanha o ponteiro durante um arrasto.
Label {
    padding: 4

    background: Rectangle {
        radius: Theme.radiusSmall
        color: Theme.ghostBackground
        border.color: Theme.dropValid
    }
}

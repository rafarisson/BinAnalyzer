import QtQuick
import QtQuick.Controls.Basic

// O rótulo que acompanha o ponteiro durante um arrasto.
Label {
    padding: 4

    background: Rectangle {
        color: Theme.ghostBackground
        border.color: Theme.dropValid
    }
}

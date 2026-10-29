import QtQuick
import QtQuick.Effects

// Fundo dos popups, menus e diálogos: cartão elevado com borda e sombra.
Item {
    id: root

    property int radius: Theme.radiusLarge

    Rectangle {
        id: card

        anchors.fill: parent
        radius: root.radius
        color: Theme.raised
        border.color: Theme.border
        visible: false
    }

    MultiEffect {
        anchors.fill: card
        source: card
        autoPaddingEnabled: true
        shadowEnabled: true
        shadowColor: Theme.shadow
        shadowOpacity: Theme.shadowOpacity
        shadowBlur: 0.9
        shadowVerticalOffset: 6
    }
}

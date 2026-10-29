import QtQuick
import QtQuick.Controls.Basic

// Grupo de botões exclusivos num bloco só (SegmentedButton). Os filhos são os segmentos.
Item {
    id: root

    default property alias segments: row.data

    // Os segmentos dividem a largura toda; sem isso cada um tem a largura do próprio conteúdo.
    property bool stretch: false

    implicitWidth: row.implicitWidth
    implicitHeight: 26

    ButtonGroup {
        buttons: row.children
    }

    Row {
        id: row

        // Lido pelos SegmentedButton.
        readonly property bool stretch: root.stretch

        width: root.stretch ? root.width : implicitWidth
        height: parent.height
    }

    // O contorno único fica por cima dos segmentos.
    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusMedium
        color: "transparent"
        border.color: Theme.borderStrong
    }
}

import QtQuick
import QtQuick.Controls.Basic

// Aviso rápido no pé da janela; some sozinho.
Rectangle {
    id: root

    function show(text, isError) {
        label.text = text
        color = isError ? Theme.errorBackground : Theme.warningBackground
        label.color = isError ? Theme.errorText : Theme.warningText
        visible = true
        hideTimer.restart()
    }

    visible: false
    radius: 4
    border.color: Qt.darker(color, 1.15)
    implicitWidth: Math.min(label.implicitWidth + 2 * Theme.cellPadding, 700)
    implicitHeight: label.implicitHeight + 2 * Theme.pagePadding

    Label {
        id: label

        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Timer {
        id: hideTimer

        interval: 5000
        onTriggered: root.visible = false
    }
}

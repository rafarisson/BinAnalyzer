pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// Avisos empilhados no pé da janela. Os avisos comuns somem sozinhos; os erros ficam até serem
// dispensados (botão ✕ ou clique).
Column {
    id: root

    readonly property int maxToasts: 4

    function show(text, isError) {
        if (toasts.count >= maxToasts)
            toasts.remove(0)
        toasts.append({ "message": text, "isError": isError })
    }

    spacing: 6

    ListModel {
        id: toasts
    }

    Repeater {
        model: toasts

        delegate: Rectangle {
            id: toast

            required property int index
            required property string message
            required property bool isError

            radius: Theme.radiusMedium
            color: isError ? Theme.errorBackground : Theme.warningBackground
            border.color: isError ? Theme.errorBorder : Theme.warningBorder
            implicitWidth: row.implicitWidth + 2 * Theme.pagePadding
            implicitHeight: row.implicitHeight + 2 * Theme.pagePadding

            RowLayout {
                id: row

                anchors.fill: parent
                anchors.margins: Theme.pagePadding

                Icon {
                    text: Icons.alert
                    font.pixelSize: 16
                    color: toast.isError ? Theme.errorText : Theme.warningText
                    Layout.leftMargin: Theme.spacingXs
                }

                Label {
                    text: toast.message
                    color: toast.isError ? Theme.errorText : Theme.warningText
                    wrapMode: Text.WordWrap
                    Layout.maximumWidth: 640
                    Layout.fillWidth: true
                }

                IconButton {
                    glyph: Icons.close
                    implicitWidth: 24
                    implicitHeight: 24
                    onClicked: toasts.remove(toast.index)
                }
            }

            NumberAnimation on opacity {
                from: 0
                to: 1
                duration: Theme.animNormal
            }

            Timer {
                interval: 5000
                running: !toast.isError
                onTriggered: toasts.remove(toast.index)
            }
        }
    }
}

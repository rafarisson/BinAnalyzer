import QtQuick

import Bina.App

// Marca o ponto de soltura sobre uma linha: uma linha antes ou depois do alvo, ou um contorno
// quando a soltura é "dentro". Fica vermelho se a soltura não é válida.
Rectangle {
    id: root

    property real rowTop: 0
    property real rowHeight: 0
    property int zone: SchemaTreeModel.Onto
    property bool valid: false
    property real leftInset: 0      // recuo da linha quando a soltura é dentro de um grupo

    readonly property bool onto: zone === SchemaTreeModel.Onto

    x: leftInset
    y: zone === SchemaTreeModel.Before ? rowTop - 1
       : zone === SchemaTreeModel.After ? rowTop + rowHeight - 1 : rowTop
    height: onto ? rowHeight : 2
    color: onto ? "transparent" : border.color
    border.width: onto ? 2 : 0
    border.color: valid ? Theme.dropValid : Theme.dropInvalid
    opacity: valid ? 1 : 0.6
}

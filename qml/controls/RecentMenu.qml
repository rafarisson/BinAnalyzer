pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

// Submenu com a lista de arquivos recentes: escolher um emite chosen(path). Sem arquivos, mostra um
// item desabilitado.
BinaMenu {
    id: root

    property var paths: []

    signal chosen(string path)

    function fileName(path) {
        const parts = String(path).split(/[\/]/)
        return parts[parts.length - 1]
    }

    Instantiator {
        // Uma entrada vazia representa "nenhum arquivo recente".
        model: root.paths.length > 0 ? root.paths : [""]

        delegate: BinaMenuItem {
            required property string modelData

            text: modelData === "" ? qsTr("(nenhum)") : root.fileName(modelData)
            enabled: modelData !== ""
            onTriggered: root.chosen(modelData)

            ToolTip.visible: hovered && modelData !== ""
            ToolTip.delay: 600
            ToolTip.text: modelData
        }

        onObjectAdded: (index, object) => root.insertItem(index, object)
        onObjectRemoved: (index, object) => root.removeItem(object)
    }
}

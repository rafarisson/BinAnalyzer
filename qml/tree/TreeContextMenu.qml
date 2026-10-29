pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import Bina.App

// Menu de contexto genérico para um model com actions(index) e trigger(index, id, text): as entradas
// (texto e habilitado) e a execução vêm do model, que também decide o que o campo permite. Uma entrada
// com 'prompt' precisa de um texto: o menu avisa por textRequested() e quem o usa pede o texto.
BinaMenu {
    id: root

    required property SchemaTreeModel itemModel

    property var targetIndex
    property list<var> entries

    signal textRequested(var index, string actionId, string prompt)

    function openFor(index, parentItem, x, y) {
        const list = itemModel.actions(index)
        if (list.length === 0)
            return

        targetIndex = index
        entries = list
        popup(parentItem, x, y)
    }

    // Bloqueia a árvore enquanto aberto e fecha ao clicar fora.
    modal: true
    dim: false

    Instantiator {
        model: root.entries

        delegate: BinaMenuItem {
            required property var modelData

            text: modelData.text
            enabled: modelData.enabled
            onTriggered: {
                if (modelData.prompt)
                    root.textRequested(root.targetIndex, modelData.id, modelData.prompt)
                else
                    root.itemModel.trigger(root.targetIndex, modelData.id)
            }
        }

        onObjectAdded: (index, object) => root.insertItem(index, object)
        onObjectRemoved: (index, object) => root.removeItem(object)
    }
}

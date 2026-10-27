import QtQuick
import QtQuick.Controls.Basic
import QtQml.Models

// Editor de texto de uma célula. Quem o hospeda chama commit() ao confirmar.
TextField {
    id: root

    required property AbstractItemModel itemModel
    required property var itemIndex

    function commit() {
        itemModel.setData(itemIndex, text, Qt.EditRole)
    }

    text: String(itemModel.data(itemIndex, Qt.EditRole))
    focus: true

    Component.onCompleted: selectAll()
}

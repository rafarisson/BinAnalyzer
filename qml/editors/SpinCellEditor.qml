import QtQuick
import QtQuick.Controls.Basic
import QtQml.Models

// Editor numérico de uma célula. Cada alteração pelas setas é aplicada na hora; um valor digitado
// é aplicado ao confirmar.
SpinBox {
    id: root

    required property AbstractItemModel itemModel
    required property var itemIndex
    property int minimum: 1
    property int maximum: 1000000

    function commit() {
        itemModel.setData(itemIndex, valueFromText((contentItem as TextInput).text, locale), Qt.EditRole)
    }

    from: minimum
    to: maximum
    editable: true
    value: Number(itemModel.data(itemIndex, Qt.EditRole))
    focus: true

    onValueModified: itemModel.setData(itemIndex, value, Qt.EditRole)
}

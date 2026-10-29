import QtQuick
import QtQuick.Controls.Basic
import QtQml.Models

// Editor de lista de uma célula. 'choices' é uma lista de {value, text} (role "choices" do model);
// escolher uma opção confirma a edição.
BinaComboBox {
    id: root

    required property AbstractItemModel itemModel
    required property var itemIndex
    property list<var> choices

    signal finished()

    function commit() {
        itemModel.setData(itemIndex, currentValue, Qt.EditRole)
    }

    model: choices
    textRole: "text"
    valueRole: "value"
    focus: true

    onActivated: {
        commit()
        finished()
    }

    Component.onCompleted: {
        currentIndex = Math.max(0, indexOfValue(String(itemModel.data(itemIndex, Qt.EditRole))))
        popup.open()
    }
}

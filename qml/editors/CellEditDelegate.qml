pragma ComponentBehavior: Bound

import QtQuick
import QtQml.Models

// Editor de célula para TableView.editDelegate. O model diz que editor abrir (role "editorKind":
// "text", "combo" ou "spin") e com quais opções ("choices", "minimum", "maximum"), então o mesmo
// delegate serve a qualquer coluna e a qualquer model que siga essas roles.
FocusScope {
    id: root

    required property TableView view    // a view que hospeda a edição (para fechá-la)
    required property var itemIndex     // índice da célula
    required property var cellData      // o "model" do delegate (acesso às roles por nome)

    readonly property AbstractItemModel itemModel: view.model as AbstractItemModel

    // Os editores têm todos commit(), mas o Loader só conhece o tipo do item como QObject.
    // qmllint disable missing-property
    TableView.onCommit: loader.item.commit()
    // qmllint enable missing-property

    Loader {
        id: loader

        anchors.fill: parent
        focus: true
        sourceComponent: {
            switch (root.cellData.editorKind) {
            case "combo": return comboEditor
            case "spin": return spinEditor
            case "readonly": return readOnlyEditor
            default: return textEditor
            }
        }
    }

    Component {
        id: textEditor

        TextCellEditor {
            itemModel: root.itemModel
            itemIndex: root.itemIndex
        }
    }

    Component {
        id: comboEditor

        ComboCellEditor {
            itemModel: root.itemModel
            itemIndex: root.itemIndex
            choices: root.cellData.choices
            onFinished: root.view.closeEditor()
        }
    }

    // Só seleciona e copia; o model recusa a gravação.
    Component {
        id: readOnlyEditor

        TextCellEditor {
            itemModel: root.itemModel
            itemIndex: root.itemIndex
            readOnly: true
            selectByMouse: true
            font.family: Theme.monoFont
        }
    }

    Component {
        id: spinEditor

        SpinCellEditor {
            itemModel: root.itemModel
            itemIndex: root.itemIndex
            minimum: root.cellData.minimum
            maximum: root.cellData.maximum
        }
    }
}

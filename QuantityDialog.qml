import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dialog
    title: "Изменение количества"
    standardButtons: Dialog.Ok | Dialog.Cancel
    anchors.centerIn: parent
    width: 300

    property int productIndex: -1
    property bool isAdd: true

    function openDialog(index, add) {
        productIndex = index
        isAdd = add
        quantityField.text = "1"
        dialog.open()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            text: isAdd ? "Добавить количество:" : "Израсходовать количество:"
            wrapMode: Text.Wrap
        }

        TextField {
            id: quantityField
            placeholderText: "Введите количество"
            validator: IntValidator { bottom: 1; top: 1000 }
            Layout.fillWidth: true
        }
    }

    onAccepted: {
        var amount = parseInt(quantityField.text)
        if (amount > 0) {
            if (isAdd) {
                bridge.addProductQuantity(productIndex, amount)
            } else {
                bridge.removeProductQuantity(productIndex, amount)
            }
        }
    }
}
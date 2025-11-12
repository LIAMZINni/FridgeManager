import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FridgeManager 1.0

ApplicationWindow {
    id: mainWindow
    width: 900
    height: 700
    visible: true
    title: "Учет продуктов ресторана"
    minimumWidth: 800
    minimumHeight: 600

    // Диалог результата
    Popup {
        id: resultDialog
        width: 500
        height: 200
        modal: true
        focus: true
        anchors.centerIn: parent
        
        background: Rectangle {
            color: "white"
            border.color: "#3498db"
            border.width: 2
            radius: 10
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15
            
            Label {
                id: dialogTitle
                text: "Результат"
                font.bold: true
                font.pixelSize: 18
                Layout.alignment: Qt.AlignHCenter
            }
            
            Label {
                id: dialogMessage
                text: ""
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            
            Button {
                text: "OK"
                Layout.alignment: Qt.AlignHCenter
                onClicked: resultDialog.close()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#f8f9fa" }
            GradientStop { position: 1.0; color: "#e9ecef" }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15

            // Заголовок
            Label {
                text: "📦 Учет продуктов холодильника"
                font.pixelSize: 28
                font.bold: true
                color: "#2c3e50"
                Layout.alignment: Qt.AlignHCenter
            }

            // Статус БД
            Rectangle {
                Layout.fillWidth: true
                height: 40
                color: fridgeManager.databaseConnected ? "#d4edda" : "#f8d7da"
                radius: 5
                border.color: fridgeManager.databaseConnected ? "#c3e6cb" : "#f5c6cb"

                Label {
                    text: fridgeManager.databaseStatus
                    color: fridgeManager.databaseConnected ? "#155724" : "#721c24"
                    font.pixelSize: 14
                    anchors.centerIn: parent
                }
            }

            // Список продуктов
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "white"
                border.color: "#dee2e6"
                radius: 8

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true

                    ListView {
                        id: productList
                        model: fridgeManager.products
                        spacing: 2

                        delegate: Rectangle {
                            width: productList.width
                            height: 70
                            color: index % 2 === 0 ? "#f8f9fa" : "#ffffff"

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 15

                                // Название продукта
                                Label {
                                    text: modelData.name
                                    font.bold: true
                                    font.pixelSize: 16
                                    color: "#2c3e50"
                                    Layout.preferredWidth: 120
                                }

                                // Количества
                                Column {
                                    Layout.preferredWidth: 120
                                    spacing: 2

                                    Label {
                                        text: "В наличии: " + modelData.currentQuantity
                                        font.pixelSize: 12
                                        color: "#495057"
                                    }

                                    Label {
                                        text: "Норма: " + modelData.normQuantity
                                        font.pixelSize: 12
                                        color: "#6c757d"
                                    }
                                }

                                // Статус заказа
                                Label {
                                    text: modelData.needsOrder ? 
                                          "⚠️ Нужен заказ: " + modelData.orderQuantity : 
                                          "✅ Достаточно"
                                    color: modelData.needsOrder ? "#dc3545" : "#28a745"
                                    font.bold: modelData.needsOrder
                                    Layout.fillWidth: true
                                }

                                // Кнопки управления
                                Row {
                                    spacing: 5
                                    Layout.alignment: Qt.AlignRight

                                    Button {
                                        text: "+"
                                        width: 40
                                        height: 30
                                        onClicked: fridgeManager.addProductQuantity(index, 1)
                                    }

                                    Button {
                                        text: "-"
                                        width: 40
                                        height: 30
                                        onClicked: fridgeManager.removeProductQuantity(index, 1)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Панель управления
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                // Информация о путях
                Column {
                    Layout.fillWidth: true
                    spacing: 5

                    Label {
                        text: "Пути сохранения:"
                        font.bold: true
                        color: "#495057"
                    }

                    Label {
                        text: "• Домашняя папка: " + fridgeManager.getDefaultHomePath()
                        font.pixelSize: 10
                        color: "#6c757d"
                    }

                    Label {
                        text: "• Документы: " + fridgeManager.getDefaultDocumentsPath()
                        font.pixelSize: 10
                        color: "#6c757d"
                    }
                }

                // Кнопки сохранения
                Column {
                    spacing: 5

                    Button {
                        text: "💾 Сохранить в домашнюю папку"
                        onClicked: {
                            var result = fridgeManager.generateOrder();
                            dialogMessage.text = result;
                            resultDialog.open();
                        }
                    }

                    Button {
                        text: "📁 Выбрать место сохранения"
                        onClicked: {
                            var result = fridgeManager.saveOrderToCustomLocation();
                            dialogMessage.text = result;
                            resultDialog.open();
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        console.log("✅ FridgeManager loaded successfully!");
        console.log("Home path:", fridgeManager.getDefaultHomePath());
        console.log("Documents path:", fridgeManager.getDefaultDocumentsPath());
    }
}
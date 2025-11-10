import QtQuick 2.15
import QtQuick.Window 2.15
import Qt.labs.platform 1.1 as Platform

Window {
    id: mainWindow
    width: 800
    height: 600
    visible: true
    title: "Учет продуктов ресторана"

    // Диалог сохранения файла
    Platform.FileDialog {
        id: fileDialog
        title: "Сохранить заявку для поставщика"
        folder: Platform.StandardPath.writableLocation(Platform.StandardPath.DocumentsLocation)
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: ["Текстовые файлы (*.txt)", "Все файлы (*)"]
        
        onAccepted: {
            var result = fridgeManager.generateOrder(fileDialog.file.toString().replace("file:///", ""));
            resultDialog.message = result;
            resultDialog.visible = true;
        }
        
        onRejected: {
            resultDialog.message = "Сохранение отменено";
            resultDialog.visible = true;
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#f0f8ff" }
            GradientStop { position: 1.0; color: "#e6f3ff" }
        }

        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15

            Text {
                text: "📦 Учет продуктов холодильника"
                font.pixelSize: 28
                font.bold: true
                color: "#2c3e50"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            // Список продуктов
            ListView {
                id: productList
                width: parent.width
                height: parent.height - 150
                model: fridgeManager.products
                spacing: 10

                delegate: Rectangle {
                    width: productList.width
                    height: 80
                    color: index % 2 === 0 ? "#ffffff" : "#f8f9fa"
                    border.color: "#dee2e6"
                    radius: 8

                    Row {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 20

                        Text {
                            text: modelData.name
                            font.bold: true
                            font.pixelSize: 18
                            color: "#2c3e50"
                            width: 120
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Column {
                            spacing: 5
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: "В наличии: " + modelData.currentQuantity
                                font.pixelSize: 14
                                color: "#34495e"
                            }

                            Text {
                                text: "Норма: " + modelData.normQuantity
                                font.pixelSize: 14
                                color: "#7f8c8d"
                            }
                        }

                        Text {
                            text: modelData.needsOrder ? 
                                  "⚠️ Нужен заказ: " + modelData.orderQuantity : 
                                  "✅ Достаточно"
                            color: modelData.needsOrder ? "#e74c3c" : "#27ae60"
                            font.bold: modelData.needsOrder
                            width: 180
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Row {
                            spacing: 10
                            anchors.verticalCenter: parent.verticalCenter

                            Rectangle {
                                width: 40
                                height: 40
                                color: "#27ae60"
                                radius: 5

                                Text {
                                    text: "+"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 18
                                    anchors.centerIn: parent
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: fridgeManager.addProductQuantity(index, 1)
                                }
                            }

                            Rectangle {
                                width: 40
                                height: 40
                                color: "#e74c3c"
                                radius: 5

                                Text {
                                    text: "-"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 18
                                    anchors.centerIn: parent
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: fridgeManager.removeProductQuantity(index, 1)
                                }
                            }
                        }
                    }
                }
            }

            // Кнопка формирования заявки
            Rectangle {
                width: 200
                height: 50
                color: "#3498db"
                radius: 10
                anchors.horizontalCenter: parent.horizontalCenter

                Text {
                    text: "📋 Сформировать заявку"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 16
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        // Устанавливаем имя файла по умолчанию
                        fileDialog.currentFile = "file:///" + fridgeManager.getDocumentsPath() + "/" + fridgeManager.getDefaultFileName();
                        fileDialog.open();
                    }
                }
            }
        }
    }

    // Диалог результата
    Rectangle {
        id: resultDialog
        width: 500
        height: 200
        color: "white"
        border.color: "#3498db"
        border.width: 3
        radius: 15
        visible: false
        anchors.centerIn: parent

        property string message: ""

        Column {
            anchors.centerIn: parent
            spacing: 20
            width: parent.width - 40

            Text {
                id: dialogTitle
                text: resultDialog.message.startsWith("Успех") ? "✅ Успех!" : "❌ Ошибка"
                font.pixelSize: 22
                font.bold: true
                color: resultDialog.message.startsWith("Успех") ? "#27ae60" : "#e74c3c"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: resultDialog.message
                font.pixelSize: 14
                color: "#34495e"
                wrapMode: Text.Wrap
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }

            Rectangle {
                width: 120
                height: 40
                color: "#3498db"
                radius: 8
                anchors.horizontalCenter: parent.horizontalCenter

                Text {
                    text: "OK"
                    color: "white"
                    font.bold: true
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: resultDialog.visible = false
                }
            }
        }
    }

    Component.onCompleted: {
        console.log("Приложение запущено! Продуктов: " + fridgeManager.products.count)
    }
}
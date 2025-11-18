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

    // Диалог выбора директории
    Popup {
        id: directoryDialog
        width: 600
        height: 400
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
                text: "📁 Выберите место сохранения"
                font.bold: true
                font.pixelSize: 18
                Layout.alignment: Qt.AlignHCenter
            }
            
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                
                Column {
                    width: parent.width
                    spacing: 5
                    
                    Repeater {
                        model: fridgeManager.getAvailableDirectories()
                        
                        Rectangle {
                            width: parent.width
                            height: 60
                            color: mouseArea.containsMouse ? "#e3f2fd" : "white"
                            border.color: "#bdc3c7"
                            border.width: 1
                            radius: 5
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 10
                                
                                Label {
                                    text: "📂"
                                    font.pixelSize: 16
                                }
                                
                                Column {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    
                                    Label {
                                        text: {
                                            var path = modelData;
                                            var parts = path.split('/');
                                            return parts[parts.length - 1] || "Корневая папка";
                                        }
                                        font.bold: true
                                        color: "#2c3e50"
                                    }
                                    
                                    Label {
                                        text: modelData
                                        font.pixelSize: 10
                                        color: "#7f8c8d"
                                        elide: Text.ElideLeft
                                    }
                                    
                                    Label {
                                        text: "Директория " + (fridgeManager.directoryExists(modelData) ? "существует" : "не существует")
                                        font.pixelSize: 10
                                        color: fridgeManager.directoryExists(modelData) ? "#27ae60" : "#e74c3c"
                                    }
                                }
                                
                                Button {
                                    text: "Выбрать"
                                    onClicked: {
                                        var result = fridgeManager.saveOrderToPath(modelData);
                                        dialogMessage.text = result;
                                        directoryDialog.close();
                                        resultDialog.open();
                                    }
                                }
                            }
                            
                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    var result = fridgeManager.saveOrderToPath(modelData);
                                    dialogMessage.text = result;
                                    directoryDialog.close();
                                    resultDialog.open();
                                }
                            }
                        }
                    }
                }
            }
            
            Row {
                spacing: 10
                Layout.alignment: Qt.AlignHCenter
                
                Button {
                    text: "Создать новую папку"
                    onClicked: createFolderDialog.open()
                }
                
                Button {
                    text: "Отмена"
                    onClicked: directoryDialog.close()
                }
            }
        }
    }

    // Диалог создания папки
    Popup {
        id: createFolderDialog
        width: 400
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
                text: "📁 Создать новую папку"
                font.bold: true
                font.pixelSize: 16
                Layout.alignment: Qt.AlignHCenter
            }
            
            TextField {
                id: newFolderName
                placeholderText: "Введите название папки"
                Layout.fillWidth: true
            }
            
            Label {
                text: "Папка будет создана в: " + fridgeManager.getDefaultHomePath()
                font.pixelSize: 12
                color: "#7f8c8d"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            
            Row {
                spacing: 10
                Layout.alignment: Qt.AlignHCenter
                
                Button {
                    text: "Создать"
                    onClicked: {
                        if (newFolderName.text.trim() !== "") {
                            var newPath = fridgeManager.getDefaultHomePath() + "/" + newFolderName.text.trim();
                            if (fridgeManager.createDirectory(newPath)) {
                                dialogMessage.text = "✅ Папка создана: " + newPath;
                                newFolderName.text = "";
                                createFolderDialog.close();
                                resultDialog.open();
                                // Обновляем список директорий
                                directoryCombo.model = fridgeManager.getAvailableDirectories();
                            } else {
                                dialogMessage.text = "❌ Не удалось создать папку: " + newPath;
                                resultDialog.open();
                            }
                        }
                    }
                }
                
                Button {
                    text: "Отмена"
                    onClicked: createFolderDialog.close()
                }
            }
        }
    }

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
            
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                
                TextArea {
                    id: dialogMessage
                    text: ""
                    wrapMode: Text.Wrap
                    readOnly: true
                    background: null
                }
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

            // Информация о последнем сохранении
            Rectangle {
                Layout.fillWidth: true
                height: 30
                visible: fridgeManager.lastSavePath !== ""
                color: "#e8f5e8"
                radius: 5
                border.color: "#c8e6c9"

                Label {
                    text: "📄 Последняя заявка: " + fridgeManager.lastSavePath
                    font.pixelSize: 12
                    color: "#2e7d32"
                    elide: Text.ElideMiddle
                    anchors {
                        left: parent.left
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        margins: 10
                    }
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
                            height: 80
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
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 40
                                    color: modelData.needsOrder ? "#ffeaa7" : "#d1ecf1"
                                    radius: 5
                                    border.color: modelData.needsOrder ? "#fdcb6e" : "#bee5eb"

                                    Label {
                                        text: modelData.needsOrder ? 
                                              "⚠️ Нужен заказ: " + modelData.orderQuantity + " упаковок" : 
                                              "✅ Достаточно"
                                        color: modelData.needsOrder ? "#e17055" : "#0c5460"
                                        font.bold: modelData.needsOrder
                                        anchors.centerIn: parent
                                    }
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
                        text: "Доступные пути:"
                        font.bold: true
                        color: "#495057"
                    }

                    ComboBox {
                        id: directoryCombo
                        width: 300
                        model: fridgeManager.getAvailableDirectories()
                        onCurrentTextChanged: {
                            if (currentText) {
                                directoryInfo.text = "Выбрано: " + currentText + 
                                    "\nСуществует: " + (fridgeManager.directoryExists(currentText) ? "✅ Да" : "❌ Нет")
                            }
                        }
                    }

                    Label {
                        id: directoryInfo
                        text: "Выберите директорию из списка"
                        font.pixelSize: 11
                        color: "#6c757d"
                        wrapMode: Text.Wrap
                    }
                }

                // Кнопки сохранения
                Column {
                    spacing: 5

                    Button {
                        text: "💾 Быстрое сохранение"
                        onClicked: {
                            var result = fridgeManager.generateOrder();
                            dialogMessage.text = result;
                            resultDialog.open();
                        }
                    }

                    Button {
                        text: "📁 Выбрать папку"
                        onClicked: {
                            directoryDialog.open();
                        }
                    }

                    Button {
                        text: "💾 Сохранить в выбранную"
                        onClicked: {
                            if (directoryCombo.currentText) {
                                var result = fridgeManager.saveOrderToPath(directoryCombo.currentText);
                                dialogMessage.text = result;
                                resultDialog.open();
                            } else {
                                dialogMessage.text = "❌ Сначала выберите папку из списка";
                                resultDialog.open();
                            }
                        }
                    }

                    Button {
                        text: "📁 Создать папку"
                        onClicked: {
                            createFolderDialog.open();
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
        console.log("Available directories:", fridgeManager.getAvailableDirectories());
        
        // Автоматически выбираем первую доступную директорию
        if (directoryCombo.count > 0) {
            directoryCombo.currentIndex = 0;
        }
    }
}

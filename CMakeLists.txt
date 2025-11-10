cmake_minimum_required(VERSION 3.16)

project(FridgeManager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Ищем Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Quick Qml Sql)

# Ищем PostgreSQL
find_package(PostgreSQL REQUIRED)

set(Qt6_DIR "D:/Qt/6.10.0/msvc2022_64/lib/cmake/Qt6")

# Явно указываем пути к Qt
set(CMAKE_PREFIX_PATH "D:/Qt/6.10.0/msvc2022_64")
# Ищем Qt6 (версия 6.10)
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Quick 
    Qml 
    LabsPlatform  # Добавляем для Qt.labs.platform
    Sql
    ${POSTGRESQL_LIBRARY}
)

# Подключаем библиотеки
target_link_libraries(FridgeManager
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    Qt6::LabsPlatform
    Qt6::Sql  
    
    
)
# Для Qt6 нужно указать дополнительные настройки
qt_standard_project_setup()
# Добавляем ресурсы
qt_add_resources(FridgeManager "qml"
    PREFIX "/"
    FILES
        Main.qml
        QuantityDialog.qml
)
# Указываем include директории
target_include_directories(FridgeManager PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}  # Для сгенерированных protobuf заголовков
)

# Добавляем include директории
target_include_directories(FridgeManager PRIVATE ${PostgreSQL_INCLUDE_DIRS})

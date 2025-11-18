cmake_minimum_required(VERSION 3.16)

project(FridgeManager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую обработку Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Ищем Qt5 компоненты
find_package(Qt5 COMPONENTS Core Quick Qml Sql REQUIRED)

# Ищем PostgreSQL
find_package(PostgreSQL REQUIRED)

find_package(Protobuf REQUIRED)

# Генерируем C++ код из .proto файлов
protobuf_generate_cpp(PROTO_SRCS PROTO_HDS product.proto)

message(STATUS "Qt5 Core found: ${Qt5Core_FOUND}")
message(STATUS "Qt5 Quick found: ${Qt5Quick_FOUND}")
message(STATUS "Qt5 Qml found: ${Qt5Qml_FOUND}")
message(STATUS "Qt5 Sql found: ${Qt5Sql_FOUND}")
message(STATUS "PostgreSQL found: ${PostgreSQL_FOUND}")

# Создаем исполняемый файл
add_executable(FridgeManager
    main.cpp
    DatabaseManager.cpp
    DatabaseManager.h
    ProtobufSerializer.cpp
    ProtobufSerializer.h
    ${PROTO_SRCS} ${PROTO_HDS}
)

# Настройки для QML
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_QML_DEBUG")

# Подключаем библиотеки
target_link_libraries(FridgeManager
    Qt5::Core
    Qt5::Quick
    Qt5::Qml
    Qt5::Sql
    ${PostgreSQL_LIBRARIES}
    protobuf
)

# Подключаем заголовочные файлы
target_include_directories(FridgeManager PRIVATE ${PostgreSQL_INCLUDE_DIRS}
${CMAKE_CURRENT_BINARY_DIR})

# Устанавливаем путь к QML импортам
target_compile_definitions(FridgeManager PRIVATE 
    QT_QML_LIB
    QT_QUICK_LIB
)

# Для Linux: устанавливаем пути к плагинам Qt
if(UNIX AND NOT APPLE)
    # Получаем путь к Qt5
    get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    execute_process(
        COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_PLUGINS
        OUTPUT_VARIABLE QT_INSTALL_PLUGINS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_QML
        OUTPUT_VARIABLE QT_INSTALL_QML
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Устанавливаем переменные окружения для runtime
    add_custom_command(TARGET FridgeManager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Setting Qt environment variables..."
    )
endif()

cmake_minimum_required(VERSION 3.16)

project(FridgeManager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую обработку Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Указываем возможные пути к Qt5
set(CMAKE_PREFIX_PATH 
    "/usr/lib/x86_64-linux-gnu/cmake/Qt5"
    "/usr/local/qt5"
    ${CMAKE_PREFIX_PATH}
)

# Ищем Qt5 компоненты
find_package(Qt5 COMPONENTS Core Quick Qml Sql REQUIRED)

# Ищем PostgreSQL
find_package(PostgreSQL REQUIRED)

message(STATUS "Qt5 version: ${Qt5_VERSION}")
message(STATUS "Qt5 Core found: ${Qt5Core_FOUND}")
message(STATUS "Qt5 Quick found: ${Qt5Quick_FOUND}")
message(STATUS "Qt5 Qml found: ${Qt5Qml_FOUND}")
message(STATUS "PostgreSQL found: ${PostgreSQL_FOUND}")

# Создаем исполняемый файл
add_executable(FridgeManager
    main.cpp
    DatabaseManager.cpp
    DatabaseManager.h
)

# Подключаем библиотеки
target_link_libraries(FridgeManager
    Qt5::Core
    Qt5::Quick
    Qt5::Qml
    Qt5::Sql
    ${PostgreSQL_LIBRARIES}
)

# Подключаем заголовочные файлы
target_include_directories(FridgeManager PRIVATE ${PostgreSQL_INCLUDE_DIRS})
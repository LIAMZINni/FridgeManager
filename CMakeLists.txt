cmake_minimum_required(VERSION 3.16)

project(FridgeManager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую обработку Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Ищем Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Quick Qml Sql)

# Ищем PostgreSQL
find_package(PostgreSQL REQUIRED)

message(STATUS "Qt5 version: ${Qt5_VERSION}")
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
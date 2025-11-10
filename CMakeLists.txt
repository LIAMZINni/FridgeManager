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

# Создаем исполняемый файл
add_executable(FridgeManager
    main.cpp
    DatabaseManager.cpp
    DatabaseManager.h
)

# Подключаем библиотеки
target_link_libraries(FridgeManager
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    Qt6::Sql
    PostgreSQL::PostgreSQL
)

# Добавляем include директории
target_include_directories(FridgeManager PRIVATE ${PostgreSQL_INCLUDE_DIRS})

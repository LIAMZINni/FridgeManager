cmake_minimum_required(VERSION 3.16)

project(FridgeManager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую обработку Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Ищем зависимости
find_package(Qt5 COMPONENTS Core Quick Qml Sql REQUIRED)
find_package(PostgreSQL REQUIRED)

# Ручной поиск Protobuf
find_path(PROTOBUF_INCLUDE_DIR
    NAMES google/protobuf/message.h
    PATHS /usr/include /usr/local/include
)

find_library(PROTOBUF_LIBRARY
    NAMES protobuf
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
)

if(PROTOBUF_INCLUDE_DIR AND PROTOBUF_LIBRARY)
    message(STATUS "Found Protobuf include: ${PROTOBUF_INCLUDE_DIR}")
    message(STATUS "Found Protobuf library: ${PROTOBUF_LIBRARY}")
    set(Protobuf_FOUND TRUE)
    set(Protobuf_INCLUDE_DIRS ${PROTOBUF_INCLUDE_DIR})
    set(Protobuf_LIBRARIES ${PROTOBUF_LIBRARY})
else()
    message(WARNING "Protobuf not found, building without Protobuf support")
    set(Protobuf_FOUND FALSE)
endif()

# Генерируем protobuf файлы
if(Protobuf_FOUND)
    execute_process(
        COMMAND protoc --cpp_out=${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/product.proto
        RESULT_VARIABLE PROTO_RESULT
    )
    
    if(PROTO_RESULT EQUAL 0)
        message(STATUS "Successfully generated protobuf files")
        set(PROTO_SRCS ${CMAKE_CURRENT_BINARY_DIR}/product.pb.cc)
        set(PROTO_HDS ${CMAKE_CURRENT_BINARY_DIR}/product.pb.h)
    else()
        message(WARNING "Failed to generate protobuf files")
        set(Protobuf_FOUND FALSE)
    endif()
endif()

# Создаем исполняемый файл
add_executable(FridgeManager
    main.cpp
    DatabaseManager.cpp
    DatabaseManager.h
    ProtobufSerializer.cpp
    ProtobufSerializer.h
)

# Добавляем protobuf файлы если они есть
if(Protobuf_FOUND)
    target_sources(FridgeManager PRIVATE ${PROTO_SRCS})
endif()

# Подключаем библиотеки
target_link_libraries(FridgeManager
    Qt5::Core
    Qt5::Quick
    Qt5::Qml
    Qt5::Sql
    ${PostgreSQL_LIBRARIES}
)

# Добавляем protobuf библиотеки если есть
if(Protobuf_FOUND)
    target_link_libraries(FridgeManager ${Protobuf_LIBRARIES})
    target_include_directories(FridgeManager PRIVATE ${Protobuf_INCLUDE_DIRS})
endif()

# Подключаем заголовочные файлы
target_include_directories(FridgeManager PRIVATE 
    ${PostgreSQL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Добавляем определение для условной компиляции
if(Protobuf_FOUND)
    target_compile_definitions(FridgeManager PRIVATE USE_PROTOBUF)
endif()

message(STATUS "Qt5 found: YES")
message(STATUS "PostgreSQL found: YES")
message(STATUS "Protobuf support: ${Protobuf_FOUND}")

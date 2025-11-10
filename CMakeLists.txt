# CMakeList.txt: проект CMake для FridgeManager; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.16)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("FridgeManager")

# Настройка стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Включаем автоматическую обработку Qt (MOC, UIC, RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTORCC ON)


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

# Ручное указание путей PostgreSQL (ЗАМЕНИ НА СВОЙ ПУТЬ!)
set(POSTGRESQL_ROOT "D:/posgesSQL")
set(POSTGRESQL_INCLUDE_DIR "${POSTGRESQL_ROOT}/include")
set(POSTGRESQL_LIBRARY "${POSTGRESQL_ROOT}/lib/libpq.lib")

message(STATUS "PostgreSQL include: ${POSTGRESQL_INCLUDE_DIR}")
message(STATUS "PostgreSQL library: ${POSTGRESQL_LIBRARY}")


# Проверяем существование файлов
if(EXISTS "${POSTGRESQL_INCLUDE_DIR}" AND EXISTS "${POSTGRESQL_LIBRARY}")
    message(STATUS "PostgreSQL found at: ${POSTGRESQL_ROOT}")
    set(PostgreSQL_FOUND TRUE)
else()
    message(WARNING "PostgreSQL not found at specified path: ${POSTGRESQL_ROOT}")
    set(PostgreSQL_FOUND FALSE)
endif()








# Добавьте источник в исполняемый файл этого проекта.
add_executable (FridgeManager  "DatabaseManager.cpp"   "main.cpp"
    
    "qml.qrc"
    "DatabaseManager.h"
    
)

# Подключаем Qt6 модули
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




if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET FridgeManager PROPERTY CXX_STANDARD 20)
endif()

# TODO: Добавьте тесты и целевые объекты, если это необходимо.


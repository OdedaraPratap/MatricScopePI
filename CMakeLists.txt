cmake_minimum_required(VERSION 3.16)
project(MatricScopePI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql PrintSupport SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql PrintSupport SerialPort)
find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio imgcodecs)

add_executable(MatricScopePI
    src/main.cpp
    src/AutoWindow.cpp
    src/AutoWindow.h
    src/MeasurementEngine.cpp
    src/MeasurementEngine.h
    src/RuleStore.cpp
    src/RuleStore.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/ApplicationDialogs.cpp
    src/ApplicationDialogs.h
    src/HistoryStore.cpp
    src/HistoryStore.h
)

target_include_directories(MatricScopePI PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(MatricScopePI PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::SerialPort
    ${OpenCV_LIBS}
)

install(TARGETS MatricScopePI RUNTIME DESTINATION bin)
install(FILES DiamondRules.xml DESTINATION share/MatricScopePI OPTIONAL)

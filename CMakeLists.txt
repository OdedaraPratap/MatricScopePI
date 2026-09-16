cmake_minimum_required(VERSION 3.16)
project(MatricScopePI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)
find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio imgcodecs)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GPIOD QUIET libgpiod)
endif()

option(MATRIC_USE_MVCAMERA "Use the Hikrobot MVS/MvCameraControl camera SDK" ON)
if(MATRIC_USE_MVCAMERA)
    find_path(MVCAMERA_INCLUDE_DIR MvCameraControl.h
        HINTS /opt/MVS/include /opt/MVS/Samples/64/include ENV MVCAMERA_ROOT
        PATH_SUFFIXES include)
    find_library(MVCAMERA_LIBRARY NAMES MvCameraControl
        HINTS /opt/MVS/lib/aarch64 /opt/MVS/lib/64 ENV MVCAMERA_ROOT
        PATH_SUFFIXES lib lib/aarch64 lib/64)
endif()

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
    src/MvCameraBackend.cpp
    src/MvCameraBackend.h
    src/GpioController.cpp
    src/GpioController.h
)

if(MVCAMERA_INCLUDE_DIR AND MVCAMERA_LIBRARY)
    target_compile_definitions(MatricScopePI PRIVATE MATRIC_HAS_MVCAMERA=1)
    target_include_directories(MatricScopePI PRIVATE ${MVCAMERA_INCLUDE_DIR})
    target_link_libraries(MatricScopePI PRIVATE ${MVCAMERA_LIBRARY})
    message(STATUS "Hikrobot MvCameraControl backend enabled")
else()
    message(WARNING "MvCameraControl SDK not found; using the OpenCV/V4L2 fallback")
endif()

if(GPIOD_FOUND)
    target_compile_definitions(MatricScopePI PRIVATE MATRIC_HAS_GPIOD=1)
    target_include_directories(MatricScopePI PRIVATE ${GPIOD_INCLUDE_DIRS})
    target_link_directories(MatricScopePI PRIVATE ${GPIOD_LIBRARY_DIRS})
    target_link_libraries(MatricScopePI PRIVATE ${GPIOD_LIBRARIES})
else()
    message(WARNING "libgpiod not found; GPIO bin output will be disabled")
endif()

target_include_directories(MatricScopePI PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(MatricScopePI PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    ${OpenCV_LIBS}
)

install(TARGETS MatricScopePI RUNTIME DESTINATION bin)
install(FILES DiamondRules.xml DESTINATION share/MatricScopePI OPTIONAL)

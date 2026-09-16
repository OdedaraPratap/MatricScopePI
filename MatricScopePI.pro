QT += core gui widgets sql

TEMPLATE = app
TARGET = MatricScopePI
CONFIG += c++11 link_pkgconfig
CONFIG -= app_bundle

DESTDIR = $$OUT_PWD/bin
OBJECTS_DIR = $$OUT_PWD/obj
MOC_DIR = $$OUT_PWD/moc
RCC_DIR = $$OUT_PWD/rcc
UI_DIR = $$OUT_PWD/ui

PKGCONFIG += opencv4

INCLUDEPATH += $$PWD/src

SOURCES += \
    src/main.cpp \
    src/AutoWindow.cpp \
    src/MeasurementEngine.cpp \
    src/RuleStore.cpp \
    src/SettingsDialog.cpp \
    src/ApplicationDialogs.cpp \
    src/HistoryStore.cpp \
    src/MvCameraBackend.cpp \
    src/GpioController.cpp

HEADERS += \
    src/AutoWindow.h \
    src/MeasurementEngine.h \
    src/RuleStore.h \
    src/SettingsDialog.h \
    src/ApplicationDialogs.h \
    src/HistoryStore.h \
    src/MvCameraBackend.h \
    src/GpioController.h

OTHER_FILES += \
    README-Qt.md \
    DiamondRules.xml \
    CMakeLists.txt

packagesExist(libgpiod) {
    PKGCONFIG += libgpiod
    DEFINES += MATRIC_HAS_GPIOD=1
    message(Raspberry Pi GPIO output enabled with libgpiod)
} else {
    warning(libgpiod was not found; GPIO bin output will be disabled)
}

# The Hikrobot Linux ARM MVS installer normally uses /opt/MVS. Qt Creator can
# override this by defining MVCAMERA_ROOT in Projects > Build Environment.
MVCAMERA_SDK_ROOT = $$(MVCAMERA_ROOT)
isEmpty(MVCAMERA_SDK_ROOT): MVCAMERA_SDK_ROOT = /opt/MVS

MVCAMERA_HEADER = $$MVCAMERA_SDK_ROOT/include/MvCameraControl.h
exists($$MVCAMERA_HEADER) {
    INCLUDEPATH += $$MVCAMERA_SDK_ROOT/include

    MVCAMERA_LIBDIR = $$MVCAMERA_SDK_ROOT/lib
    exists($$MVCAMERA_SDK_ROOT/lib/aarch64/libMvCameraControl.so) {
        MVCAMERA_LIBDIR = $$MVCAMERA_SDK_ROOT/lib/aarch64
    } else: exists($$MVCAMERA_SDK_ROOT/lib/armhf/libMvCameraControl.so) {
        MVCAMERA_LIBDIR = $$MVCAMERA_SDK_ROOT/lib/armhf
    } else: exists($$MVCAMERA_SDK_ROOT/lib/64/libMvCameraControl.so) {
        MVCAMERA_LIBDIR = $$MVCAMERA_SDK_ROOT/lib/64
    } else: exists($$MVCAMERA_SDK_ROOT/lib/32/libMvCameraControl.so) {
        MVCAMERA_LIBDIR = $$MVCAMERA_SDK_ROOT/lib/32
    }

    exists($$MVCAMERA_LIBDIR/libMvCameraControl.so) {
        DEFINES += MATRIC_HAS_MVCAMERA=1
        LIBS += -L$$MVCAMERA_LIBDIR -lMvCameraControl
        QMAKE_RPATHDIR += $$MVCAMERA_LIBDIR
        message(Hikrobot MvCameraControl enabled from $$MVCAMERA_SDK_ROOT)
    } else {
        warning(MvCameraControl header found, but libMvCameraControl.so was not found; using V4L2 fallback)
    }
} else {
    warning(Hikrobot MVS SDK not found at $$MVCAMERA_SDK_ROOT; using V4L2 fallback)
}

unix:!macx {
    target.path = /usr/local/bin
    rules.path = /usr/local/share/MatricScopePI
    rules.files = DiamondRules.xml
    INSTALLS += target rules
}

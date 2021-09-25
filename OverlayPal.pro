QT += core qml quickcontrols2
CONFIG += qmltypes
CONFIG += console
#CONFIG += qml_debug
CONFIG += c++17

QML_IMPORT_NAME = nes.overlay.optimiser
QML_IMPORT_MAJOR_VERSION = 1

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/cpp/HardwareColorsModel.cpp \
    src/cpp/main.cpp \
    src/cpp/GridLayer.cpp \
    src/cpp/ImageUtils.cpp \
    src/cpp/OverlayPalGuiBackend.cpp \
    src/cpp/OverlayOptimiser.cpp \
    src/cpp/SubProcess.cpp \
    src/cpp/SimplePaletteModel.cpp

RESOURCES += src/qml/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target qml

HEADERS += \
    src/cpp/GridLayer.h \
    src/cpp/Array2D.h \
    src/cpp/HardwareColorsModel.h \
    src/cpp/ImageUtils.h \
    src/cpp/OverlayPalGuiBackend.h \
    src/cpp/OverlayOptimiser.h \
    src/cpp/SubProcess.h \
    src/cpp/SimplePaletteModel.h

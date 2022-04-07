QT += core qml quickcontrols2
CONFIG += qmltypes
CONFIG += console
#CONFIG += qml_debug
CONFIG += c++17

# enable this to have a sane debugging experience.
OVERLAYPAL_FEATURES += copy_cmpl

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
    src/cpp/Export.cpp \
    src/cpp/HardwareColorsModel.cpp \
    src/cpp/OverlayPalApp.cpp \
    src/cpp/Sprite.cpp \
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
    src/cpp/Export.h \
    src/cpp/GridLayer.h \
    src/cpp/Array2D.h \
    src/cpp/HardwareColorsModel.h \
    src/cpp/ImageUtils.h \
    src/cpp/OverlayPalApp.h \
    src/cpp/OverlayPalGuiBackend.h \
    src/cpp/OverlayOptimiser.h \
    src/cpp/Sprite.h \
    src/cpp/SubProcess.h \
    src/cpp/SimplePaletteModel.h

macx {
    # copy nes palette to $app/Contents/MacOS/nespalettes
    NES_PALETTE_FILES.files = $$files($$PWD/nespalettes/*.pal)
    NES_PALETTE_FILES.path = Contents/MacOS/nespalettes
    QMAKE_BUNDLE_DATA += NES_PALETTE_FILES
    # copy cmpl to $app/Contents/MacOS/nespalettes
    CMPL_FILES.files = $$files($$PWD/src/cmpl/*.cmpl)
    CMPL_FILES.path = Contents/MacOS
    QMAKE_BUNDLE_DATA += CMPL_FILES
    contains(OVERLAYPAL_FEATURES, copy_cmpl) {
        # copy cmpl
        CMPL_BIN_FILES.files = $$files($$PWD/Cmpl-2-0-1-macOs-Intel/Cmpl2/Coliop.app/Contents/MacOS/*)
        CMPL_BIN_FILES.path = Contents/MacOS/Cmpl
        QMAKE_BUNDLE_DATA += CMPL_BIN_FILES
    }
}

unix:!macx {
    # copy nespalettes to $OUT_PWD/nespalettes
    eval($$OUT_PWD != $$PWD) {
        QMAKE_POST_LINK += $$QMAKE_COPY_DIR $$shell_quote($$PWD/nespalettes) $$shell_quote($$OUT_PWD/nespalettes) $$escape_expand(\\n\\t)
    }
    # Copy .cmpl files to $OUT_PWD/
    QMAKE_POST_LINK += $$QMAKE_COPY_FILE $$shell_quote($$PWD/src/cmpl/FirstPass.cmpl) $$shell_quote($$OUT_PWD/FirstPass.cmpl) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$QMAKE_COPY_FILE $$shell_quote($$PWD/src/cmpl/SecondPass.cmpl) $$shell_quote($$OUT_PWD/SecondPass.cmpl) $$escape_expand(\\n\\t)
    # Conditionally copy entire cmpl directory only if copy_cmpl is set
    contains(OVERLAYPAL_FEATURES, copy_cmpl) {
        QMAKE_POST_LINK += $$QMAKE_COPY_DIR $$shell_quote($$PWD/Cmpl-2-0-1-linux64) $$shell_quote($$OUT_PWD/Cmpl) $$escape_expand(\\n\\t)
    }
}

win32 {
    build_pass: CONFIG(debug, debug|release) {
        OUT_PWD_WIN = $$OUT_PWD/debug
    }
    else: build_pass {
        OUT_PWD_WIN = $$OUT_PWD/release
    }
    # Replace / with \ to make QMAKE_COPY_* work
    PWD_WIN=$$PWD
    PWD_WIN ~= s?/?\\?g
    OUT_PWD_WIN ~= s?/?\\?g
    # copy nespalettes to $OUT_PWD_WIN/nespalettes
    QMAKE_POST_LINK += $$QMAKE_COPY_DIR $$shell_quote($$PWD_WIN\\nespalettes) $$shell_quote($$OUT_PWD_WIN\\nespalettes) $$escape_expand(\\n\\t)
    # Copy .cmpl files to $OUT_PWD_WIN/
    QMAKE_POST_LINK += $$QMAKE_COPY_FILE $$shell_quote($$PWD_WIN\\src\\cmpl\\FirstPass.cmpl) $$shell_quote($$OUT_PWD_WIN\\FirstPass.cmpl) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$QMAKE_COPY_FILE $$shell_quote($$PWD_WIN\\src\\cmpl\\SecondPass.cmpl) $$shell_quote($$OUT_PWD_WIN\\SecondPass.cmpl) $$escape_expand(\\n\\t)
    # Conditionally copy entire cmpl directory only if copy_cmpl is set
    contains(OVERLAYPAL_FEATURES, copy_cmpl) {
        QMAKE_POST_LINK += $$QMAKE_COPY_DIR $$shell_quote($$PWD_WIN\\Cmpl-2-0-1-win) $$shell_quote($$OUT_PWD_WIN\\Cmpl) $$escape_expand(\\n\\t)
    }
}

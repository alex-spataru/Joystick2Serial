#-------------------------------------------------------------------------------
# Opciones de compilacion
#-------------------------------------------------------------------------------

UI_DIR = uic
MOC_DIR = moc
RCC_DIR = qrc
OBJECTS_DIR = obj

isEmpty(PREFIX) {
    PREFIX = /usr
}

CONFIG += c++11
CONFIG += silent

#-------------------------------------------------------------------------------
# Configuracion de Qt
#-------------------------------------------------------------------------------

TEMPLATE = app
TARGET = joystick2serial

QT += gui
QT += svg
QT += core
QT += widgets
QT += serialport

QTPLUGIN += qsvg
CONFIG += qtquickcompiler
CONFIG += utf8_source

#-------------------------------------------------------------------------------
# Archivos
#-------------------------------------------------------------------------------

include($$PWD/lib/Libraries.pri)

HEADERS += \
    src/HAL_Driver.h \
    src/MainWindow.h \
    src/Serial.h \
    src/Utilities.h

SOURCES += \
    src/MainWindow.cpp \
    src/Serial.cpp \
    src/Utilities.cpp \
    src/main.cpp

FORMS += \
    src/MainWindow.ui

RESOURCES += \
    res/Resources.qrc

#-------------------------------------------------------------------------------
# Opciones especificas a cada plataforma
#-------------------------------------------------------------------------------

win32* {
    TARGET = Joystick2Serial                             # Change target name
    RC_FILE = deploy/windows/resources/info.rc           # Set applicaiton icon
    OTHER_FILES += deploy/windows/nsis/setup.nsi         # Setup script
}

macx* {
    TARGET = Joystick2Serial                             # Change target name
    ICON = deploy/macOS/icon.icns                        # icon file
    RC_FILE = deploy/macOS/icon.icns                     # icon file
    QMAKE_INFO_PLIST = deploy/macOS/info.plist           # Add info.plist file
    CONFIG += sdk_no_version_check                       # Avoid warnings with Big Sur
}

linux:!android {
    target.path = $$PREFIX/bin                           # Set binary installation path
    icon.path = $$PREFIX/share/pixmaps                   # icon instalation path
    desktop.path = $$PREFIX/share/applications           # *.desktop instalation path
    icon.files += deploy/linux/*.svg                     # Add application icon
    desktop.files += deploy/linux/*.desktop              # Add *.desktop file
    INSTALLS += target desktop icon                      # make install targets
}

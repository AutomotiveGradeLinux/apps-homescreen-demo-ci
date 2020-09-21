TEMPLATE = app
TARGET = homescreen-demo-ci

QT = qml quick dbus websockets gui-private
CONFIG += c++11 link_pkgconfig wayland-scanner
#DESTDIR = .
DESTDIR = $${OUT_PWD}/../package/root/bin
PKGCONFIG += wayland-client json-c

CONFIG(release, debug|release) {
    QMAKE_POST_LINK = $(STRIP) --strip-unneeded $(TARGET)
}

SOURCES += src/main.cpp \
	   src/shell.cpp

HEADERS += src/shell.h

RESOURCES += qml/qml.qrc images/images.qrc

WAYLANDCLIENTSOURCES += \
    protocol/agl-shell.xml

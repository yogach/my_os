TEMPLATE = app
CONFIG += console
CONFIG -= qt

INCLUDEPATH += ../my_os/

DEFINES += DTFSER

SOURCES += main.c \
    hdraw.c \
    ../my_os/utility.c \
    ../my_os/fs.c

HEADERS += \
    hdraw.h \
    ../my_os/utility.h \
    ../my_os/fs.h


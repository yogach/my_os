TEMPLATE = app
CONFIG += console
CONFIG -= qt

INCLUDEPATH += ../my_os/

DEFINES += DTFSER

SOURCES += main.c \
    hdraw.c \
    ../my_os/utility.c \
    ../my_os/fs.c \
    ../my_os/list.c

HEADERS += \
    hdraw.h \
    ../my_os/utility.h \
    ../my_os/fs.h \
    ../my_os/list.h


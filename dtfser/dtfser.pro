TEMPLATE = app
CONFIG += console
CONFIG -= qt

INCLUDEPATH += ../my_os/

SOURCES += main.c \
    hdraw.c \
    ../my_os/utility.c

HEADERS += \
    hdraw.h \
    ../my_os/utility.h


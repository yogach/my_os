TEMPLATE = app
CONFIG += console
CONFIG -= qt

INCLUDEPATH += ../D.T.OS/

SOURCES += main.c \
    hdraw.c \
    ../D.T.OS/utility.c

HEADERS += \
    hdraw.h \
    ../D.T.OS/utility.h


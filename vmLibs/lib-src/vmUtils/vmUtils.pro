#-------------------------------------------------
#
# Project created by QtCreator 2018-02-07T20:40:13
#
#-------------------------------------------------

QT       += widgets

TARGET = vmUtils
TEMPLATE = lib

DEFINES += VMUTILS_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    configops.cpp \
    fast_library_functions.cpp \
    fileops.cpp \
    tristatetype.cpp \
    vmcompress.cpp \
    crashrestore.cpp \
    vmfilemonitor.cpp \
    vmtextfile.cpp

HEADERS += \
        vmutils_global.h \ 
    configops.h \
    fast_library_functions.h \
    fileops.h \
    tristatetype.h \
    vmcompress.h \
    vmutils_global.h \
    crashrestore.h \
    vmfilemonitor.h \
    vmtextfile.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DEFINES += DEBUG QT_USE_QSTRINGBUILDER QT_USE_FAST_CONCATENATION QT_USE_FAST_OPERATOR_PLUS
QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -fomit-frame-pointer -funroll-loops -Ofast
QMAKE_CXXFLAGS_DEBUG += -g
QMAKE_CXXFLAGS += -Werror -Wall -Wextra -pedantic -std=c++14 -finput-charset=UTF-8 -fexec-charset=UTF-8

LIBS += -L/usr/lib -lbz2
INCLUDEPATH += $$PWD/../../common $$PWD/../../lib-src

unix:!macx: LIBS += -L$$PWD/../build-vmStringRecord-Desktop-Debug/ -lvmStringRecord
DEPENDPATH += $$PWD/../vmStringRecord

unix:!macx: LIBS += -L$$PWD/../build-vmNumbers-Desktop-Debug/ -lvmNumbers
DEPENDPATH += $$PWD/../vmNumbers

unix:!macx: LIBS += -L$$PWD/../build-vmNotify-Desktop-Debug/ -lvmNotify
DEPENDPATH += $$PWD/../vmNotify

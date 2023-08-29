#-------------------------------------------------
#
# Project created by QtCreator 2018-02-04T12:05:19
#
#-------------------------------------------------

QT       += widgets

TARGET = vmWidgets
TEMPLATE = lib

DEFINES += VMWIDGETS_LIBRARY

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
    vmlistitem.cpp \
    vmlistwidget.cpp \
    vmtableitem.cpp \
    vmtableutils.cpp \
    vmtablewidget.cpp \
    vmwidget.cpp \
    vmwidgets.cpp \
    texteditwithcompleter.cpp \
    vmcheckedtableitem.cpp \
    vmlinefilter.cpp \
    wordhighlighter.cpp \
    spellcheck.cpp

HEADERS += \
        vmwidgets_global.h \ 
    vmlistitem.h \
    vmlistwidget.h \
    vmtableitem.h \
    vmtableutils.h \
    vmtablewidget.h \
    vmwidget.h \
    vmwidgets.h \
    texteditwithcompleter.h \
    vmcheckedtableitem.h \
    vmlinefilter.h \
    wordhighlighter.h \
    spellcheck.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DEFINES += DEBUG QT_USE_QSTRINGBUILDER QT_USE_FAST_CONCATENATION QT_USE_FAST_OPERATOR_PLUS QT_NO_DEBUG

QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -fomit-frame-pointer -funroll-loops -Ofast
QMAKE_CXXFLAGS_DEBUG += -g
QMAKE_CXXFLAGS += -Werror -Wall -Wextra -pedantic -std=c++14 -finput-charset=UTF-8 -fexec-charset=UTF-8

INCLUDEPATH += -I/usr/include $$PWD/../../common $$PWD/../../lib-src
LIBS += -L/usr/lib -lhunspell

unix:!macx: LIBS += -L$$PWD/../build-vmCalculator-Desktop-Debug/ -lvmCalculator
DEPENDPATH += $$PWD/../vmCalculator

unix:!macx: LIBS += -L$$PWD/../build-vmNumbers-Desktop-Debug/ -lvmNumbers
DEPENDPATH += $$PWD/../vmNumbers

unix:!macx: LIBS += -L$$PWD/../build-vmStringRecord-Desktop-Debug/ -lvmStringRecord
DEPENDPATH += $$PWD/../vmStringRecord

unix:!macx: LIBS += -L$$PWD/../build-vmUtils-Desktop-Debug/ -lvmUtils
DEPENDPATH += $$PWD/../vmUtils

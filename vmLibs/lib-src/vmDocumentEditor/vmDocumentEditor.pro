#-------------------------------------------------
#
# Project created by QtCreator 2018-02-09T17:22:59
#
#-------------------------------------------------

QT       += widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += printsupport

TARGET = vmDocumentEditor
TEMPLATE = lib

DEFINES += VMDOCUMENTEDITOR_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    documenteditor.cpp \
    documenteditorwindow.cpp \
    texteditor.cpp \
    reportgenerator.cpp \
    spreadsheet.cpp

HEADERS += \
        vmdocumenteditor_global.h \ 
    documenteditor.h \
    documenteditorwindow.h \
    texteditor.h \
    vmdocumenteditor_global.h \
    reportgenerator.h \
    spreadsheet.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DEFINES += QT_USE_QSTRINGBUILDER QT_USE_FAST_CONCATENATION QT_USE_FAST_OPERATOR_PLUS
DEFINES_DEBUG += DEBUG QT_DEBUG
DEFINES_RELEASE += QT_NO_DEBUG
QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -fomit-frame-pointer -funroll-loops -Ofast
QMAKE_CXXFLAGS_DEBUG += -g
QMAKE_CXXFLAGS += -Werror -Wall -Wextra -pedantic -std=c++14 -finput-charset=UTF-8 -fexec-charset=UTF-8

INCLUDEPATH += $$PWD/../../common $$PWD/../../lib-src

unix:!macx: LIBS += -L$$PWD/../build-vmStringRecord-Desktop-Debug/ -lvmStringRecord
DEPENDPATH += $$PWD/../vmStringRecord

unix:!macx: LIBS += -L$$PWD/../build-vmUtils-Desktop-Debug/ -lvmUtils
DEPENDPATH += $$PWD/../vmUtils

unix:!macx: LIBS += -L$$PWD/../build-vmNotify-Desktop-Debug/ -lvmNotify
DEPENDPATH += $$PWD/../vmNotify

unix:!macx: LIBS += -L$$PWD/../build-vmWidgets-Desktop-Debug/ -lvmWidgets
DEPENDPATH += $$PWD/../vmWidgets

unix:!macx: LIBS += -L$$PWD/../build-vmCalculator-Desktop-Debug/ -lvmCalculator
DEPENDPATH += $$PWD/../vmCalculator

unix:!macx: LIBS += -L$$PWD/../build-vmNumbers-Desktop-Debug/ -lvmNumbers
DEPENDPATH += $$PWD/../vmNumbers

RESOURCES += \
    resources.qrc

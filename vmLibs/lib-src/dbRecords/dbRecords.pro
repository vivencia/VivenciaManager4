#-------------------------------------------------
#
# Project created by QtCreator 2018-02-07T19:13:25
#
#-------------------------------------------------

QT       -= gui
QT		 += sql

TARGET = dbRecords
TEMPLATE = lib

DEFINES += DBRECORDS_LIBRARY

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
    dbsupplies.cpp \
    generaltable.cpp \
    inventory.cpp \
    job.cpp \
    machinesrecord.cpp \
    payment.cpp \
    purchases.cpp \
    quickproject.cpp \
    supplierrecord.cpp \
    userrecord.cpp \
    vivenciadb.cpp \
    client.cpp \
    companypurchases.cpp \
    completerrecord.cpp \
    dbrecord.cpp \
    dbcalendar.cpp \
    dblistitem.cpp \
    completers.cpp \
    dbtablewidget.cpp

HEADERS += \
        dbrecords_global.h \ 
    dbsupplies.h \
    generaltable.h \
    inventory.h \
    job.h \
    machinesrecord.h \
    payment.h \
    purchases.h \
    quickproject.h \
    supplierrecord.h \
    userrecord.h \
    vivenciadb.h \
    client.h \
    companypurchases.h \
    completerrecord.h \
    dbrecord.h \
    dbcalendar.h \
    dblistitem.h \
    completers.h \
    dbtablewidget.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DEFINES += DEBUG QT_USE_QSTRINGBUILDER QT_USE_FAST_CONCATENATION QT_USE_FAST_OPERATOR_PLUS
QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -fomit-frame-pointer -funroll-loops -Ofast
QMAKE_CXXFLAGS_DEBUG += -g
QMAKE_CXXFLAGS += -Werror -Wall -Wextra -pedantic -std=c++14 -finput-charset=UTF-8 -fexec-charset=UTF-8

INCLUDEPATH += $$PWD/../../common $$PWD/../../lib-src

unix:!macx: LIBS += -L$$PWD/../build-vmNumbers-Desktop-Debug/ -lvmNumbers
DEPENDPATH += $$PWD/../vmNumbers

unix:!macx: LIBS += -L$$PWD/../build-vmStringRecord-Desktop-Debug/ -lvmStringRecord
DEPENDPATH += $$PWD/../vmStringRecord

unix:!macx: LIBS += -L$$PWD/../build-vmUtils-Desktop-Debug/ -lvmUtils
DEPENDPATH += $$PWD/../vmUtils

unix:!macx: LIBS += -L$$PWD/../build-vmNotify-Desktop-Debug/ -lvmNotify
DEPENDPATH += $$PWD/../vmNotify

unix:!macx: LIBS += -L$$PWD/../build-vmWidgets-Desktop-Debug/ -lvmWidgets
DEPENDPATH += $$PWD/../vmWidgets

RESOURCES += \
    listitem_icons.qrc

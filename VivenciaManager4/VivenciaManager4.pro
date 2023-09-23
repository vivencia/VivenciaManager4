#-------------------------------------------------
#
# Project created by QtCreator 2018-02-14T18:03:41
#
#-------------------------------------------------

QT       += core gui sql widgets

#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VivenciaManager4
TEMPLATE = app

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
	backupdialog.cpp \
        calendarviewui.cpp \
	companypurchases.cpp \
	configdialog.cpp \
        contactsmanagerwidget.cpp \
        dbstatistics.cpp \
        dbtableview.cpp \
        estimates.cpp \
        machinesdlg.cpp \
	main.cpp \
	mainwindow.cpp \
	newprojectdialog.cpp \
	searchui.cpp \
	separatewindow.cpp \
	suppliersdlg.cpp \
	system_init.cpp

HEADERS += \
	backupdialog.h \
        calendarviewui.h \
	companypurchases.h \
	configdialog.h \
	contactsmanagerwidget.h \
	dbstatistics.h \
	dbtableview.h \
	estimates.h \
	machinesdlg.h \
	mainwindow.h \
	newprojectdialog.h \
	searchui.h \
	separatewindow.h \
	suppliersdlg.h \
	system_init.h \
        global.h

FORMS += \
        calendarviewui.ui \
        mainwindow.ui \
        backupdialog.ui \
        companypurchases.ui \
        configdialog.ui

RESOURCES += \
        resources.qrc

DISTFILES += \
        project_files/spreadsheet.xlsx \
        project_files/project.docx \
        project_files/vivencia.jpg \
        project_files/vivencia_report_logo.jpg

unix:!macx: LIBS += -L$$PWD/../vmLibs/libs/ -ldbRecords -lvmUtils -lvmWidgets -lvmStringRecord \
									-lvmNumbers -lvmTaskPanel -lvmDocumentEditor -lvmCalculator \
									-lvmNotify

DEPENDPATH += $$PWD/../vmLibs/libs $$PWD/../vmLibs/common $$PWD/../vmLibs/lib-src
INCLUDEPATH += $$PWD/../vmLibs/lib-src $$PWD/../vmLibs/common

DEFINES += DEBUG QT_USE_QSTRINGBUILDER QT_USE_FAST_CONCATENATION QT_USE_FAST_OPERATOR_PLUS QT_NO_DEBUG

QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -fomit-frame-pointer -funroll-loops -Ofast
#QMAKE_CXXFLAGS_DEBUG += -g -fvar-tracking-assignments-toggle
QMAKE_CXXFLAGS_DEBUG += -g
#QMAKE_CXXFLAGS += -g -fvar-tracking-assignments-toggle
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic -std=c++11 -finput-charset=UTF-8 -fexec-charset=UTF-8

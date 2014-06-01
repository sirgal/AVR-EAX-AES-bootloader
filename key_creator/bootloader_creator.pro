#-------------------------------------------------
#
# Project created by QtCreator 2013-08-01T21:02:12
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = bootloader_creator
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    eaxencrypt.cpp \
    intelhexprocessor.cpp

HEADERS  += mainwindow.h \
    eaxencrypt.h \
    intelhexprocessor.h

FORMS    += mainwindow.ui

win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../cryptopp562/debug/ -llibcryptopp562
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../cryptopp562/release/ -llibcryptopp562

INCLUDEPATH += $$PWD/../../cryptopp562
DEPENDPATH += $$PWD/../../cryptopp562

win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../cryptopp562/debug/libcryptopp562.lib
win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../cryptopp562/release/libcryptopp562.lib

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

mac:CONFIG -= app_bundle

#CONFIG += release
QT = core

# Input
HEADERS += decompress.h
SOURCES += main.cpp decompress.cpp

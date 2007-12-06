TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

mac:CONFIG -= app_bundle

#CONFIG += release
QT = core

include(../callgrind_decompress/callgrind_decompress.pri)

# Input
SOURCES += main.cpp

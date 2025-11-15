TEMPLATE = app
TARGET = tetris

QT += core gui widgets

CONFIG += c++11

SOURCES += \
    main.cpp \
    tetrisgame.cpp

HEADERS += \
    tetrisgame.h

# Link against the input event library for gamepad support
LIBS += -lpthread

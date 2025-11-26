QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    battle/battle_system.cpp \
    battle/battle_animations.cpp\
    introscreen.cpp \
    main.cpp \
    mainwindow.cpp \
    map/map_loader.cpp \

    gamepadthread.cpp

HEADERS += \
    battle/battle_system.h \
    introscreen.h \
    mainwindow.h \
    map/map_loader.h
    gamepadthread.h

FORMS += \
    mainwindow.ui

RESOURCES += assets.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    assets/back1.png \
    assets/back2.png \
    assets/back3.png \
    assets/background.png \
    assets/collision.png \
    assets/front1.png \
    assets/front2.png \
    assets/front3.png \
    assets/intro/copyright/copyright_01.png \
    assets/intro/fire/fire_01.png \
    assets/intro/fire/fire_02.png \
    assets/intro/fire/fire_03.png \
    assets/intro/fire/fire_04.png \
    assets/intro/fire/fire_05.png \
    assets/intro/fire/fire_06.png \
    assets/intro/fire/fire_07.png \
    assets/intro/fire/fire_08.png \
    assets/intro/fire/fire_09.png \
    assets/intro/fire/fire_10.png \
    assets/intro/grass/grass_01.png \
    assets/intro/grass/grass_02.png \
    assets/intro/grass/grass_03.png \
    assets/intro/grass/grass_04.png \
    assets/intro/grass/grass_05.png \
    assets/intro/grass/grass_06.png \
    assets/intro/press/press_off.png \
    assets/intro/press/press_on.png \
    assets/intro/title/title_01.png \
    assets/intro/title/title_02.png \
    assets/intro/title/title_03.png \
    assets/intro/title/title_04.png \
    assets/left1.png \
    assets/left2.png \
    assets/left3.png \
    assets/right1.png \
    assets/right2.png \
    assets/right3.png

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += game_logic/json

SOURCES += \
    battle/battle_system.cpp \
    battle/battle_animations.cpp \
    game_logic/Attack.cpp \
    game_logic/Bag.cpp \
    game_logic/Battle.cpp \
    game_logic/Item.cpp \
    game_logic/Player.cpp \
    game_logic/Pokemon.cpp \
    game_logic/PokemonData.cpp \
    game_logic/Type.cpp \
    game_logic/jsoncpp.cpp \
    introscreen.cpp \
    main.cpp \
    mainwindow.cpp \
    map/map_loader.cpp \
    gamepadthread.cpp

HEADERS += \
    battle/battle_system.h \
    game_logic/Attack.h \
    game_logic/Bag.h \
    game_logic/Battle.h \
    game_logic/Item.h \
    game_logic/Player.h \
    game_logic/Pokemon.h \
    game_logic/PokemonData.h \
    game_logic/Type.h \
    introscreen.h \
    mainwindow.h \
    map/map_loader.h \
    gamepadthread.h

# FORMS removed - not using Qt Designer UI files
# FORMS += \
#     mainwindow.ui

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

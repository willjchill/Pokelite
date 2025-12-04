QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += Battle/Battle_logic/json

SOURCES += \
    General/main.cpp \
    General/window.cpp \
    General/gamepad.cpp \
    General/uart_comm.cpp \
    Intro_Screen/introscreen.cpp \
    Overworld/Overworld.cpp \
    Overworld/Player_OW.cpp \
    Overworld/Camera_OW.cpp \
    Overworld/Map_OW.cpp \
    Overworld/Menu_OW.cpp \
    Overworld/map_loader.cpp \
    Battle/GUI_BT.cpp \
    Battle/BattleState_BT.cpp \
    Battle/Animations_BT.cpp \
    Battle/Player_BT.cpp \
    Battle/Pokemon_BT.cpp \
    Battle/Battle_logic/Attack.cpp \
    Battle/Battle_logic/Bag.cpp \
    Battle/Battle_logic/Battle.cpp \
    Battle/Battle_logic/Item.cpp \
    Battle/Battle_logic/Player.cpp \
    Battle/Battle_logic/Pokemon.cpp \
    Battle/Battle_logic/PokemonData.cpp \
    Battle/Battle_logic/Type.cpp \
    Battle/Battle_logic/jsoncpp.cpp

HEADERS += \
    General/window.h \
    General/gamepad.h \
    General/uart_comm.h \
    Intro_Screen/introscreen.h \
    Overworld/Overworld.h \
    Overworld/Player_OW.h \
    Overworld/Camera_OW.h \
    Overworld/Map_OW.h \
    Overworld/Menu_OW.h \
    Overworld/map_loader.h \
    Battle/GUI_BT.h \
    Battle/BattleState_BT.h \
    Battle/Animations_BT.h \
    Battle/Player_BT.h \
    Battle/Pokemon_BT.h \
    Battle/Battle_logic/Attack.h \
    Battle/Battle_logic/Bag.h \
    Battle/Battle_logic/Battle.h \
    Battle/Battle_logic/Item.h \
    Battle/Battle_logic/Player.h \
    Battle/Battle_logic/Pokemon.h \
    Battle/Battle_logic/PokemonData.h \
    Battle/Battle_logic/Type.h

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

#ifndef OVERWORLD_H
#define OVERWORLD_H

#include <QObject>
#include <QKeyEvent>
#include <QRandomGenerator>
#include "Player_OW.h"
#include "Camera_OW.h"
#include "Map_OW.h"
#include "Menu_OW.h"
#include "../Battle/Battle_logic/Player.h"

class Overworld : public QObject
{
    Q_OBJECT

public:
    Overworld(QGraphicsScene *scene, QGraphicsView *view);
    ~Overworld();

    // Map management
    void loadMap(const QString &name);
    void checkForMapExit();

    // Movement and collision
    void handleMovementInput(QKeyEvent *event);
    void handleKeyRelease(QKeyEvent *event);
    void clampPlayer();

    // Player management
    void setPlayer(Player *player);
    Player* getPlayer() const { return gamePlayer; }
    void setPlayerPosition(const QPointF &pos);

    // Wild encounters
    void tryWildEncounter();

    // Overworld menu
    void showOverworldMenu();
    void hideOverworldMenu();
    void handleOverworldMenuKey(QKeyEvent *event);
    bool isInMenu() const { return menu->isInMenu(); }

    // Getters
    Player_OW* getPlayerItem() const { return playerOW; }
    QGraphicsScene* getScene() const { return mapOW; }
    Camera_OW* getCamera() const { return cameraOW; }

signals:
    void wildEncounterTriggered();
    void pvpBattleRequested();

private:
    Map_OW *mapOW;
    Camera_OW *cameraOW;
    Player_OW *playerOW;
    Menu_OW *menu;
    QGraphicsView *view;

    // Movement
    float speed = 2.0f;
    bool isMoving = false;

    // Player reference
    Player *gamePlayer = nullptr;
};

#endif // OVERWORLD_H


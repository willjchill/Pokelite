#ifndef OVERWORLD_H
#define OVERWORLD_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QImage>
#include <QTimer>
#include <QVector>
#include <QMap>
#include <QString>
#include <QObject>
#include "map/map_loader.h"
#include "game_logic/Player.h"

class Overworld : public QObject
{
    Q_OBJECT

public:
public:
    Overworld(QGraphicsScene *scene, QGraphicsView *view);
    ~Overworld();

    // Map management
    void loadMap(const QString &name);
    void applyMap(const MapData &m);
    QString detectExitAtPlayerPosition();
    void checkForMapExit();

    // Movement and collision
    void handleMovementInput(QKeyEvent *event);
    void handleKeyRelease(QKeyEvent *event);
    bool isSolidPixel(int x, int y);
    bool isSlowPixel(int x, int y);
    bool isGrassPixel(int x, int y);
    void clampPlayer();

    // Camera
    void updateCamera();

    // Player sprite management
    void loadAnimations();
    void setPlayerDirection(const QString &direction);
    QString getCurrentDirection() const { return currentDirection; }

    // Wild encounters
    void tryWildEncounter();
    void setPlayer(Player *player) { gamePlayer = player; }
    Player* getPlayer() const { return gamePlayer; }

    // Overworld menu
    void showOverworldMenu();
    void hideOverworldMenu();
    void handleOverworldMenuKey(QKeyEvent *event);
    bool isInMenu() const { return inOverworldMenu || inOverworldBagMenu || inOverworldPokemonMenu || inOverworldPokemonListMenu; }

    // Getters
    QGraphicsPixmapItem* getPlayerItem() const { return player; }
    QGraphicsScene* getScene() const { return scene; }

private:
    QGraphicsScene *scene;
    QGraphicsView *view;
    QGraphicsPixmapItem *background;
    QGraphicsPixmapItem *player;

    // Movement
    float speed = 4.0f;
    bool isMoving = false;
    QString currentDirection = "front";
    int frameIndex = 1;
    QTimer animationTimer;
    std::map<QString, std::vector<QPixmap>> animations;

    // Maps
    QImage collisionMask;
    QImage tallGrassMask;
    QImage exitMask;
    QString currentMapName;
    MapData currentMap;

    // Player reference
    Player *gamePlayer = nullptr;

    // Overworld menu
    bool inOverworldMenu = false;
    bool inOverworldBagMenu = false;
    bool inOverworldPokemonMenu = false;
    bool inOverworldPokemonListMenu = false;  // True when showing all Pokemon to select
    int overworldMenuIndex = 0;
    int selectedSlotIndex = -1;  // Which slot (0-5) is being edited, -1 if none
    QGraphicsRectItem *overworldMenuRect = nullptr;
    QGraphicsRectItem *overworldBagMenuRect = nullptr;
    QGraphicsRectItem *overworldPokemonMenuRect = nullptr;
    QGraphicsRectItem *overworldPokemonListMenuRect = nullptr;
    QVector<QGraphicsTextItem*> overworldMenuOptions;
    QVector<QGraphicsTextItem*> overworldBagMenuOptions;
    QVector<QGraphicsTextItem*> overworldPokemonMenuOptions;
    QVector<QGraphicsTextItem*> overworldPokemonListMenuOptions;
    QGraphicsPixmapItem *overworldCursorSprite = nullptr;

    void overworldMenuSelected(int index);
    void showOverworldBagMenu();
    void showOverworldPokemonMenu();
    void showOverworldPokemonListMenu(int slotIndex);
    void hideOverworldSubMenu();
    void updateOverworldMenuCursor();
    void overworldPokemonSelected(int index);
    void overworldPokemonListSelected(int index);
    void swapOrReplacePokemon(int slotIndex, int pokemonIndex);

signals:
    void wildEncounterTriggered();
};

#endif // OVERWORLD_H


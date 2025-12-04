#include "Overworld.h"
#include <QDebug>

Overworld::Overworld(QGraphicsScene *scene, QGraphicsView *view)
    : QObject(view), view(view)
{
    Q_UNUSED(scene); // We create our own Map_OW scene instead
    
    // Create map (which is a scene)
    mapOW = new Map_OW(this);
    
    // Set the view to use our map scene
    if (view) {
        view->setScene(mapOW);
    }
    
    // Create camera (uses the existing view)
    cameraOW = new Camera_OW(view);
    
    // Create player
    playerOW = new Player_OW();
    mapOW->addItem(playerOW);
    
    // Create menu
    menu = new Menu_OW(mapOW, view, this);
    
    // Connect menu signals
    connect(menu, &Menu_OW::menuClosed, this, [this]() {
        hideOverworldMenu();
    });
    connect(menu, &Menu_OW::pvpBattleRequested, this, &Overworld::pvpBattleRequested);
}

Overworld::~Overworld()
{
    // Qt parent system handles cleanup
}

void Overworld::loadMap(const QString &name)
{
    // Remove player from scene before clearing (so it doesn't get deleted)
    if (playerOW && mapOW->items().contains(playerOW)) {
        mapOW->removeItem(playerOW);
    }
    
    mapOW->loadMap(name);
    
    // Re-add player to scene after map is loaded
    if (playerOW) {
        mapOW->addItem(playerOW);
        
        // Set player position to spawn
        MapData currentMap = mapOW->getCurrentMap();
        playerOW->setPosition(currentMap.playerSpawn);
    }
    
    if (cameraOW && playerOW) {
        cameraOW->updateCamera(playerOW);
    }
}

void Overworld::checkForMapExit()
{
    QString exitId = mapOW->detectExitAtPlayerPosition(playerOW);
    if (exitId.isEmpty()) return;

    MapData currentMap = mapOW->getCurrentMap();
    if (currentMap.exits.contains(exitId)) {
        loadMap(currentMap.exits[exitId]);
    }
}

void Overworld::handleMovementInput(QKeyEvent *event)
{
    float dx=0, dy=0;
    QString direction;

    switch(event->key()) {
    case Qt::Key_W: dy = -1; direction="back";  break;
    case Qt::Key_S: dy = +1; direction="front"; break;
    case Qt::Key_A: dx = -1; direction="left";  break;
    case Qt::Key_D: dx = +1; direction="right"; break;
    default: return;
    }

    playerOW->setDirection(direction);
    QPointF oldPos = playerOW->getPosition();
    QPointF tryPos(oldPos.x() + dx * speed,
                   oldPos.y() + dy * speed);

    int px = tryPos.x() + playerOW->boundingRect().width()/2;
    int py = tryPos.y() + playerOW->boundingRect().height() - 4;

    float finalSpeed = speed;
    if (mapOW->isSlowPixel(px, py))
        finalSpeed = speed * 0.4f;

    tryPos = QPointF(oldPos.x() + dx * finalSpeed,
                     oldPos.y() + dy * finalSpeed);

    px = tryPos.x() + playerOW->boundingRect().width()/2;
    py = tryPos.y() + playerOW->boundingRect().height() - 4;

    if (!mapOW->isSolidPixel(px, py))
        playerOW->setPosition(tryPos);

    cameraOW->updateCamera(playerOW);

    if (mapOW->isGrassPixel(px, py))
        tryWildEncounter();

    checkForMapExit();

    if (!isMoving && (dx!=0 || dy!=0)) {
        isMoving = true;
        playerOW->startAnimation();
    }

    clampPlayer();
}

void Overworld::handleKeyRelease(QKeyEvent *event)
{
    if (event->key()==Qt::Key_W ||
        event->key()==Qt::Key_A ||
        event->key()==Qt::Key_S ||
        event->key()==Qt::Key_D)
    {
        isMoving = false;
        playerOW->stopAnimation();
        cameraOW->updateCamera(playerOW);
    }
}

void Overworld::clampPlayer()
{
    QRectF bounds = playerOW->boundingRect();
    QPointF pos = playerOW->getPosition();
    QRectF mapRect = mapOW->sceneRect();

    if (pos.x() < mapRect.left()) pos.setX(mapRect.left());
    if (pos.x() > mapRect.right()-bounds.width()) pos.setX(mapRect.right()-bounds.width());

    if (pos.y() < mapRect.top()) pos.setY(mapRect.top());
    if (pos.y() > mapRect.bottom()-bounds.height()) pos.setY(mapRect.bottom()-bounds.height());

    playerOW->setPosition(pos);
}

void Overworld::tryWildEncounter()
{
    if (QRandomGenerator::global()->bounded(100) < 2) {
        emit wildEncounterTriggered();
    }
}

void Overworld::setPlayer(Player *player)
{
    gamePlayer = player;
    menu->setPlayer(player);
}

void Overworld::showOverworldMenu()
{
    menu->showMenu();
}

void Overworld::hideOverworldMenu()
{
    menu->hideMenu();
}

void Overworld::handleOverworldMenuKey(QKeyEvent *event)
{
    menu->handleKey(event);
}


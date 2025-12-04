#include "Map_OW.h"
#include <QDebug>

Map_OW::Map_OW(QObject *parent)
    : QGraphicsScene(parent), background(nullptr)
{
}

Map_OW::~Map_OW()
{
}

void Map_OW::loadMap(const QString &name)
{
    currentMapName = name;
    currentMap = MapLoader::load(name);
    applyMap(currentMap);
}

void Map_OW::applyMap(const MapData &m)
{
    clear();

    background = addPixmap(QPixmap(m.background));
    background->setZValue(0);

    if (!background->pixmap().isNull()) {
        setSceneRect(
            0, 0,
            background->pixmap().width(),
            background->pixmap().height()
        );
    }

    // Load masks
    collisionMask = QImage(m.collision);
    tallGrassMask = QImage(m.tallgrass);
    exitMask = QImage(m.exitMask);
}

bool Map_OW::isSolidPixel(int x, int y) const
{
    if (x < 0 || y < 0 ||
        x >= collisionMask.width() ||
        y >= collisionMask.height())
        return true;

    QColor p = collisionMask.pixelColor(x, y);
    return (p.red() < 30 && p.green() < 30 && p.blue() < 30);
}

bool Map_OW::isSlowPixel(int x, int y) const
{
    if (x < 0 || y < 0 ||
        x >= collisionMask.width() ||
        y >= collisionMask.height())
        return false;

    QColor p = collisionMask.pixelColor(x, y);
    return (p.blue() > 200 && p.red() < 80 && p.green() < 80);
}

bool Map_OW::isGrassPixel(int x, int y) const
{
    if (x < 0 || y < 0 ||
        x >= tallGrassMask.width() ||
        y >= tallGrassMask.height())
        return false;

    QColor px = tallGrassMask.pixelColor(x, y);
    return (px.red() > 200 && px.blue() > 200 && px.green() < 100);
}

QString Map_OW::detectExitAtPlayerPosition(QGraphicsItem *player) const
{
    if (exitMask.isNull() || !player) return "";

    int px = player->x() + player->boundingRect().width()/2;
    int py = player->y() + player->boundingRect().height() - 4;

    if (px < 0 || py < 0 || px >= exitMask.width() || py >= exitMask.height())
        return "";

    QColor c = exitMask.pixelColor(px, py);

    // EXIT COLORS:
    if (c.red() > 200 && c.green() < 50 && c.blue() < 50)
        return "house1_door";
    if (c.blue() > 200 && c.red() < 50 && c.green() < 50)
        return "cave_entrance";
    if (c.green() > 200 && c.red() < 50 && c.blue() < 50)
        return "exit_door";

    return "";
}


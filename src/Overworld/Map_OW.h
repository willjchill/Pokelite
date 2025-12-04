#ifndef MAP_OW_H
#define MAP_OW_H

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QString>
#include "map_loader.h"

class Map_OW : public QGraphicsScene
{
    Q_OBJECT

public:
    Map_OW(QObject *parent = nullptr);
    ~Map_OW();

    void loadMap(const QString &name);
    void applyMap(const MapData &m);
    
    // Collision detection
    bool isSolidPixel(int x, int y) const;
    bool isSlowPixel(int x, int y) const;
    bool isGrassPixel(int x, int y) const;
    
    // Exit detection
    QString detectExitAtPlayerPosition(QGraphicsItem *player) const;
    
    // Getters
    QString getCurrentMapName() const { return currentMapName; }
    MapData getCurrentMap() const { return currentMap; }
    QGraphicsPixmapItem* getBackground() const { return background; }

private:
    QGraphicsPixmapItem *background;
    QImage collisionMask;
    QImage tallGrassMask;
    QImage exitMask;
    QString currentMapName;
    MapData currentMap;
};

#endif // MAP_OW_H


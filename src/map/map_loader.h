#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <QString>
#include <QPointF>
#include <QMap>

struct MapData
{
    QString background;
    QString collision;
    QString tallgrass;
    QString exitMask;

    QPointF playerSpawn;

    QMap<QString, QString> exits;
};

class MapLoader
{
public:
    static MapData load(const QString &name);
};

#endif // MAP_LOADER_H

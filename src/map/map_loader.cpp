#include "map_loader.h"
#include <QDebug>

MapData MapLoader::load(const QString &name)
{
    MapData m;

    // --------------------
    // Route 1 Example Map
    // --------------------
    if (name == "route1")
    {
        m.background = ":/assets/maps/route1/background.png";
        m.collision  = ":/assets/maps/route1/collision.png";
        m.tallgrass  = ":/assets/maps/route1/tallgrass.png";
        m.exitMask   = ":/assets/maps/route1/exits.png";

        m.playerSpawn = QPointF(160, 300);

        m.exits["house1_door"] = "route1_house1";
        m.exits["cave_entrance"] = "viridian_cave";

        return m;
    }

    // ---------------------------------
    // Interior Example (House)
    // ---------------------------------
    if (name == "route1_house1")
    {
        m.background = ":/assets/maps/house1/background.png";
        m.collision  = ":/assets/maps/house1/collision.png";
        m.tallgrass  = "";
        m.exitMask   = ":/assets/maps/house1/exits.png";

        m.playerSpawn = QPointF(120, 200);

        m.exits["exit_door"] = "route1";

        return m;
    }

    qDebug() << "Unknown map:" << name;

    m.background = ":/assets/maps/default/background.png";
    m.collision  = ":/assets/maps/default/collision.png";
    m.tallgrass  = "";
    m.exitMask   = "";
    m.playerSpawn = QPointF(80, 80);

    return m;
}

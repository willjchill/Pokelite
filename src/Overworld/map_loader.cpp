#include "map_loader.h"
#include <QDebug>

MapData MapLoader::load(const QString &name)
{
    MapData m;

    // --------------------
    // Route 6 Example Map
    // --------------------
    if (name == "route6")
    {
        m.background = ":/assets/maps/route6/background.png";
        m.collision  = ":/assets/maps/route6/collision.png";
        m.tallgrass  = ":/assets/maps/route6/tallgrass.png";
        m.exitMask   = "";  // No exit mask for route6 yet

        m.playerSpawn = QPointF(160, 300);

        m.exits["house1_door"] = "route6_house1";
        m.exits["cave_entrance"] = "viridian_cave";

        return m;
    }

    // ---------------------------------
    // Interior Example (House)
    // ---------------------------------
    if (name == "route6_house1")
    {
        m.background = ":/assets/maps/house1/background.png";
        m.collision  = ":/assets/maps/house1/collision.png";
        m.tallgrass  = "";
        m.exitMask   = ":/assets/maps/house1/exits.png";

        m.playerSpawn = QPointF(120, 200);

        m.exits["exit_door"] = "route6";

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

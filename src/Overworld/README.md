# Overworld

Overworld exploration system with map loading, player movement, collision detection, and wild encounters.

## Components

### Overworld
`Overworld.h/cpp` - Main overworld controller. Coordinates map, camera, player, and menu systems. Handles movement input, collision detection, map transitions, wild encounter triggering, and menu management.

### Map_OW
`Map_OW.h/cpp` - QGraphicsScene for rendering overworld maps. Manages background rendering, collision masks, tall grass detection, and exit detection. Provides pixel-perfect collision checking for solid tiles, slow tiles, and grass tiles.

### map_loader
`map_loader.h/cpp` - Map data loading system. Loads map configuration files containing background image paths, collision masks, tall grass masks, exit masks, spawn points, and map exit connections.

### Player_OW
`Player_OW.h/cpp` - QGraphicsObject representing the player sprite in the overworld. Handles sprite animation, directional movement, and position management. Supports four-directional movement with animated sprites.

### Camera_OW
`Camera_OW.h/cpp` - Camera system that follows the player. Centers the QGraphicsView on the player position with zoom support. Maintains viewport boundaries.

### Menu_OW
`Menu_OW.h/cpp` - Overworld menu system. Provides menu interface accessible during exploration. Displays Pokemon team, allows Pokemon swapping/reordering, and provides access to PvP battle functionality.


# Battle System

Handles all battle-related functionality including turn-based combat, UI rendering, animations, and game logic.

## Core Components

### BattleState_BT
`BattleState_BT.h/cpp` - Wrapper around the Battle class providing a UI-friendly interface. Manages battle state, processes player actions, and provides getters for UI display.

### GUI_BT
`GUI_BT.h/cpp` - BattleSequence class manages the battle UI. Handles menu navigation, sprite rendering, HP bars, text display, and coordinates with the battle system. Supports both wild encounters and PvP battles.

### Animations_BT
`Animations_BT.h/cpp` - Handles battle animations including trainer throw, Pokemon entrances, menu slides, and battle reveal effects.

## Battle Logic

### Battle
`Battle_logic/Battle.h/cpp` - Core battle engine. Manages turn order, damage calculation, type effectiveness, accuracy checks, critical hits, and battle flow. Supports wild battles and PvP mode.

### Player
`Battle_logic/Player.h/cpp` - Represents a player with a team of Pokemon and a bag of items. Manages active Pokemon selection and team state.

### Pokemon
`Battle_logic/Pokemon.h/cpp` - Individual Pokemon entity with stats, moves, HP, experience, and level. Handles damage, healing, leveling, and evolution.

### Attack
`Battle_logic/Attack.h/cpp` - Move/attack representation with type, power, accuracy, PP, and category (physical/special/status).

### Bag
`Battle_logic/Bag.h/cpp` - Item inventory management system.

### Item
`Battle_logic/Item.h/cpp` - Individual item representation with name, quantity, and effects.

### Type
`Battle_logic/Type.h/cpp` - Type system with effectiveness calculations for Gen 3 Pokemon mechanics.

### PokemonData
`Battle_logic/PokemonData.h/cpp` - Data loading from JSON files. Provides species data, evolution information, level-up moves, and move metadata.

## Data Files

- `firered_pokedex.json` - Pokemon species data
- `firered_moves.json` - Move data
- `firered_full_pokedex.json` - Complete Pokedex information


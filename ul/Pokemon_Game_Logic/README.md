# Pokemon Generation 3 Battle System

A C++ implementation of Pokemon battle mechanics from Generation 3 games (specifically Pokemon Fire Red).

## Overview

This project implements a complete Pokemon battle system with accurate Generation 3 mechanics including:
- Damage calculation using the Gen 3 formula
- Type effectiveness (super effective, not very effective, no effect)
- Accuracy checks
- Critical hits (1/16 chance)
- STAB (Same-Type Attack Bonus)
- Run mechanics (different for wild vs trainer battles)
- Turn order based on Speed stat

## Classes

### Core Classes

- **`Type`**: Enum representing Pokemon types with type effectiveness calculations
- **`Attack`**: Represents a Pokemon move with power, accuracy, PP, and type
- **`Item`**: Represents items that can be used in battle (potions, etc.)
- **`Pokemon`**: Represents a Pokemon with HP, level, stats, moves, and types
- **`Bag`**: Container for items
- **`Player`**: Represents a player with a team of up to 6 Pokemon and a bag
- **`Battle`**: Manages the battle flow and mechanics

## Battle Mechanics

### Damage Formula (Generation 3)

```
Damage = (((2 * Level / 5 + 2) * BasePower * (Attack / Defense)) / 50 + 2) * Modifier
```

Where `Modifier` includes:
- **STAB**: 1.5x if move type matches Pokemon type
- **Type Effectiveness**: 2.0x (super effective), 0.5x (not very effective), 0.0x (no effect)
- **Critical Hit**: 2.0x (1/16 chance)
- **Random Factor**: 85-100% of calculated damage

### Accuracy

Moves have an accuracy value (0-100). A random number (1-100) is rolled, and if it's <= accuracy, the move hits.

### Run Mechanics

**Wild Pokemon Battles:**
```
A = ((PlayerSpeed * 32) / EnemySpeed) + (EscapeAttempts * 30)
```
If `A >= random(0-255)`, escape succeeds.

**Trainer Battles:**
Cannot run from trainer battles.

### Turn Order

Turn order is determined by Speed stat. The Pokemon with higher Speed moves first. If speeds are equal, it's random.

## Usage Example

```cpp
#include "Battle.h"
#include "Player.h"
#include "Pokemon.h"
#include "Attack.h"

// Create players
Player player1("Ash", PlayerType::HUMAN);
Player opponent("Gary", PlayerType::NPC);

// Create Pokemon
Pokemon pikachu("Pikachu", 10, Type::ELECTRIC);
pikachu.addMove(Attack("Thunderbolt", Type::ELECTRIC, 90, 100, 15, MoveCategory::SPECIAL));
player1.addPokemon(pikachu);

Pokemon squirtle("Squirtle", 10, Type::WATER);
squirtle.addMove(Attack("Water Gun", Type::WATER, 40, 100, 25, MoveCategory::SPECIAL));
opponent.addPokemon(squirtle);

// Create and start battle
Battle battle(&player1, &opponent, false);  // false = trainer battle
battle.startBattle();

// Process actions
battle.processAction(BattleAction::FIGHT);
battle.processFightAction(0);  // Use first move
```

## Building

Use the provided Makefile:

```bash
make
```

This will create the `pokemon_battle` executable.

## Battle Phases

1. **Setup**: Initial battle messages and Pokemon selection
2. **Menu**: Main battle menu (FIGHT, BAG, POKEMON, RUN)
3. **Fight Menu**: Select which move to use
4. **Bag Menu**: Select which item to use
5. **Pokemon Menu**: Select which Pokemon to switch to
6. **Executing Turn**: Moves are executed in speed order
7. **Battle End**: Battle concludes when one side is defeated or player runs

## Features

- ✅ Accurate Gen 3 damage calculation
- ✅ Type effectiveness system
- ✅ STAB bonus
- ✅ Critical hits
- ✅ Accuracy checks
- ✅ Run mechanics (wild vs trainer)
- ✅ Turn order based on Speed
- ✅ PP (Power Points) system
- ✅ Item usage (potions, etc.)
- ✅ Pokemon switching
- ✅ Battle status display

## Notes

- The stat calculation is simplified for demonstration. In the actual games, stats depend on base stats, IVs, EVs, and nature.
- Status effects (burn, paralysis, etc.) are not yet implemented but can be added.
- The type effectiveness chart includes all Gen 3 types.


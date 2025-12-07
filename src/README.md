# RPG Embedded Game

A Pokemon-style RPG game built with **C++** and **Qt Framework**, designed to run on BeagleBone Black with LCD touchscreen cape. Features turn-based battles, PvP multiplayer via UART, and smooth battle animations.

## Gameplay Features:

Battle through Pokemon encounters with turn-based combat featuring:
- **Attack** - Choose from 3 unique moves per Pokemon
- **Items** - Use potions and revives from inventory
- **Switch** - Swap Pokemon mid-battle strategically
- **Catch** - Capture wild Pokemon with Pokeballs
- **PvP** - Real time multiplayer battle with other players

Fight wild Pokemon to level up your team, or connect two BeagleBones via UART for real-time PvP battles.

## Project Structure

- **General/**: Core application infrastructure and window management
- **Intro_Screen/**: Game introduction sequence
- **Overworld/**: Overworld exploration and map system
- **Battle/**: Battle system with GUI, animations, and game logic

## Entry Point

`General/main.cpp` initializes the Qt application, displays the intro screen, and transitions to the main game window.

## Build System

- Qt project file: `QtPokemonGame.pro`
- Makefile: `Makefile`
- Resource file: `assets.qrc`


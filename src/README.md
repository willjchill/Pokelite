# RPG Embedded Game

A Pokemon-style RPG game built with Qt for embedded systems. The game features overworld exploration, turn-based battles, and multiplayer PvP support via UART communication.

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


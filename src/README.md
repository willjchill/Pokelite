# RPG Embedded Game

A Pokemon-style RPG game built with **C++** and **Qt Framework**, designed to run on BeagleBone Black with LCD touchscreen cape. Features turn-based battles, PvP multiplayer via UART, and smooth battle animations.


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


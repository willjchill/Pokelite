# RPG Embedded Game

A Pokemon-style RPG game built with **C++** and **Qt Framework**, designed to run on BeagleBone Black with LCD touchscreen cape. Features turn-based battles, PvP multiplayer via UART, and smooth battle animations.

## Gameplay Features:
![Walking Demo](misc/Walking.gif)


Battle through Pokemon encounters with turn-based combat featuring:
- **Attack** - Choose from 3 unique moves per Pokemon
- **Items** - Use potions and revives from inventory
- **Switch** - Swap Pokemon mid-battle strategically
- **Catch** - Capture wild Pokemon with Pokeballs
- **PvP** - Real time multiplayer battle with other players

Fight wild Pokemon to level up your team, or connect two BeagleBones via UART for real-time PvP battles.

### Battle Animations
- Pokeball throw animation with arc trajectory and rotation
- Attack impact effects with screen shake
- Pokemon sprite movements (lunge forward, recoil back)
- Smooth HP bar transitions with color coding
- Battle transitions and fade effects
- White flash on critical hits

### Interactive UI
- Custom battle menus with cursor navigation
- HP indicator boxes with tail pointers to Pokemon
- Move selection with type indicators
- Bag interface for items and Pokeballs
- Pokemon switching interface with team overview

### Embedded Systems Integration
- **Framebuffer Rendering** - Direct rendering to `/dev/fb0` at 60 FPS
- **Controller Input** - USB gamepad support (D-pad, A/B/X/Y, L/R, Start/Select)
- **UART Communication** - Real-time PvP battle synchronization via link cable
- **Boot Animation** - Game Boy-style startup sequence

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


==== UPDATE LOG ====

NAME, DATE, DESCRIPTION
ptnguy01, 12/5/2025, Overhaul of battle system + animations fixed. Added impact graphics for pokemon attacking + misc graphics fix to smooth transition between window frames
willjdes, 12/5/2025, compatible with /etc/inittab startup, more UART fixes
ptnguy01, 12/4/2025, Added animations of player throwing aniamtion at the start of battle. Added introduction sequence + intro lab map with tutorial allowing players to choose starting pokemon.
willjdes, 12/4/2025, Adding UART, fixing project organization A LOT
willjdes, 12/2/2025, Adding Pokeball feature and fixing GUI. 
ptnguy01, 11/24/2025, Finished up the intro sequence graphics when booting up the game
willjdes, 11/23/2025, Adding ARM Makefile. Allows userland code to be run properly on BeagleBone. Note that QT uses C++17 and game logic uses C++11.
willjdes, 11/20/2025, Further tweaking game state and fixing pokemon database. Basic attacks should work. No modifiers like poison or flinch etc exist. Also need to add "catching" and wild encounter probabilities.
willjdes, 11/19/2025, Setup pokemon JSON files, parser, battle logic, initial object designs
ptnguy01, 11/19/2025, Added animations for player sprite, added collisions for obstacles on map
ptnguy01, 11/18/2025, Uploaded src for basic movement using assets to feature/graphics branch
ptnguy01, 11/17/2025, Uploaded basic controlling movement to feature/MainGame
willjdes, 11/15/2025, Used built-in xpad.c module from bbb and tested functionality with tetris.c using QT
ptnguy01, 11/14/2025, Uploaded controller.c and Makefile - Makefile needs modification to run on BeagleBone
willjdes, 11/13/2025, Google Doc created
willjdes, 11/13/2025, Git repo created

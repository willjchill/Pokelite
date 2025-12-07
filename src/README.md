# RPG Embedded Game

A Pokemon-style RPG game built with **C++** and **Qt Framework**, designed to run on BeagleBone Black with LCD touchscreen cape. Features turn-based battles, PvP multiplayer via UART, and smooth battle animations.

## Gameplay Features:

![Walking Demo](misc/WalkingDemo.gif)

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


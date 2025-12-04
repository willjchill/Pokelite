# General

Core application infrastructure and system integration.

## Components

### main.cpp
Application entry point. Initializes Qt application, creates intro screen and main window, and handles transition between them.

### window
`window.h/cpp` - Main game window (QMainWindow). Manages QGraphicsScene and QGraphicsView for rendering. Coordinates between Overworld and Battle systems. Handles keyboard input, gamepad input, and UART communication for PvP battles.

### gamepad
`gamepad.h/cpp` - QThread that reads gamepad input from `/dev/input/event1`. Emits signals for button presses and analog stick movements. Converts gamepad events to keyboard events for game control.

### uart_comm
`uart_comm.h/cpp` - UART communication system for PvP battles. Handles serial communication on `/dev/ttyS1` at 115200 baud. Manages packet serialization/deserialization for battle synchronization. Supports finding players, battle initialization, turn synchronization, and battle end communication.


# Intro Screen

Game introduction sequence displayed at startup.

## Components

### introscreen
`introscreen.h/cpp` - QWidget that displays animated intro sequence. Manages three phases: copyright screen, grass animation, and title screen. Supports keyboard and gamepad input to skip or advance through phases. Emits `introFinished` signal when complete to transition to main game window.


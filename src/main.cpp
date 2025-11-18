#include <iostream>
#include "Game.h"

int main() {
    Game game;

    // Simulate some actions
    game.player.moveRight();
    game.player.moveUp();

    std::cout << "Player position: (" 
              << game.player.x << ", " 
              << game.player.y << ")\n";

    // Simulate game loop
    for (int i = 0; i < 5; i++) {
        game.update();
    }

    return 0;
}

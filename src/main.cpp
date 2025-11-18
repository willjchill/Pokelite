#include <iostream>
#include "Game.h"

int main() {
    Game game;

    char cmd;
    while (true) {
        std::cin >> cmd;

        if (cmd == 'w') game.player.moveUp();
        if (cmd == 's') game.player.moveDown();
        if (cmd == 'a') game.player.moveLeft();
        if (cmd == 'd') game.player.moveRight();

        game.update();

        std::cout << "Player at (" << game.player.x << ", " << game.player.y << ")\n";
    }

    return 0;
}

#pragma once
#include "Entity.h"
#include "Pokemon.h"

class Player : public Entity {
public:
    int speed = 2; // arbitrarily chosen rn -> signifies the speed (number of pixels) the player can move when press move

    Player(int x, int y);

    // Basic movement
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();

    // Inventory for player
    std:: vector<Pokemon> party;
    int pokeballs = 3; // default value
    void addPokemon(const Pokemon& p);
};
#pragma once
#include "Entity.h"
#include "Pokemon.h"
#include <vector>

class Player : public Entity {
public:
    int speed = 2;
    std::vector<Pokemon> party;
    int pokeballs = 3;

    Player(int x, int y);
    void addPokemon(const Pokemon& p);

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
};

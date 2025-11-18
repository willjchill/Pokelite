#pragma once
#include "Player.h"

class Game {
public:
    Player player;

    Game();

    // Wild encounter and map battles
    bool checkWildEncounter();
    void startBattle(const Pokemon& wild);

    void update();   // update game state


};

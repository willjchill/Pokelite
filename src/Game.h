#pragma once
#include "Player.h"
#include "NPC.h"
#include <vector>

class Game {
public:
    Player player;
    std::vector<NPC> npcs;

    Game();

    void update();
    bool checkWildEncounter();
    void startBattle(const Pokemon& wild);
    void checkNPCInteraction();
};

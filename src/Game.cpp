#include "Game.h"
#include <iostream>
#include <cstdlib>

Game::Game()
    : player(100, 100), map(50, 50) //map not implemented
{
    // Example NPC
    npcs.push_back(NPC(120, 100, "Hello trainer!"));
}

void Game::update() {
    checkNPCInteraction();

    if (checkWildEncounter()) {
        Pokemon wild("Pidgey", rand() % 3 + 2);
        startBattle(wild);
    }
}


void Game::startBattle(const Pokemon& wild) {
    std::cout << "A wild " << wild.name << " appeared!\n";
}

void Game::checkNPCInteraction() {
    for (auto& npc : npcs) {
        if (abs(player.x - npc.x) < 32 &&
            abs(player.y - npc.y) < 32)
        {
            std::cout << "NPC says: " << npc.dialog << "\n";
        }
    }
}

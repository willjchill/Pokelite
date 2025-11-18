#include "Pokemon.h"

Pokemon::Pokemon(std::string n, int lvl)
    : name(n), level(lvl)
{
    maxHp = lvl; // Modify so it fit what the game is like
    hp = maxHp;

    // Random movement arbitraliy chosen -> Modify to match game stats
    moves.push_back({"Tackle", 35, 95});
    moves.push_back({"Growl", 0, 100});
}

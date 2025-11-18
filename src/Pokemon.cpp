#include "Pokemon.h"

Pokemon::Pokemon(std::string n, int lvl)
    : name(n), level(lvl)
{
    //maxHp lvl; //  MODIFY ACCCORDING TO RULE
    hp = maxHp;

    moves.push_back({"Tackle", 35, 95});
    moves.push_back({"Growl", 0, 100});
}

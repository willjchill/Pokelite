#pragma once
#include <string>
#include <vector>

struct Move {
    std::string name;
    int power;
    int accuracy;
};

class Pokemon {
public:
    std::string name;
    int level;
    int hp;
    int maxHp;
    std::vector<Move> moves;

    Pokemon(std::string name, int level);
};

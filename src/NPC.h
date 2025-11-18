#pragma once
#include "Entity.h"
#include <string>

class NPC : public Entity {
public:
    std::string dialog;

    NPC(int x, int y, std::string d)
        : Entity(x, y, 32, 32), dialog(d) {}
};

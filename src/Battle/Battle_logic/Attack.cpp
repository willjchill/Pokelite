#include "Attack.h"
#include <algorithm>

Attack::Attack(const std::string& name, Type type, int power, int accuracy, int maxPP, MoveCategory category)
    : name(name), type(type), power(power), accuracy(accuracy), maxPP(maxPP), currentPP(maxPP), category(category) {
}

void Attack::use() {
    if (currentPP > 0) {
        currentPP--;
    }
}

void Attack::restorePP(int amount) {
    currentPP = std::min(currentPP + amount, maxPP);
}


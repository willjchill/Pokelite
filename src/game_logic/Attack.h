#ifndef ATTACK_H
#define ATTACK_H

#include "Type.h"
#include <string>

enum class MoveCategory {
    PHYSICAL,
    SPECIAL,
    STATUS
};

class Attack {
private:
    std::string name;
    Type type;
    int power;
    int accuracy;  // 0-100
    int maxPP;
    int currentPP;
    MoveCategory category;

public:
    Attack(const std::string& name, Type type, int power, int accuracy, int maxPP, MoveCategory category);
    
    // Getters
    std::string getName() const { return name; }
    Type getType() const { return type; }
    int getPower() const { return power; }
    int getAccuracy() const { return accuracy; }
    int getMaxPP() const { return maxPP; }
    int getCurrentPP() const { return currentPP; }
    MoveCategory getCategory() const { return category; }
    
    // Check if move can be used
    bool canUse() const { return currentPP > 0; }
    
    // Use the move (decrease PP)
    void use();
    
    // Restore PP (for items like Ether)
    void restorePP(int amount);
};

#endif // ATTACK_H


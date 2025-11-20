#ifndef ITEM_H
#define ITEM_H

#include <string>

enum class ItemType {
    POTION,           // Restores HP
    SUPER_POTION,     // Restores more HP
    POKE_BALL,        // For catching Pokemon
    ETHER,            // Restores PP
    REVIVE,           // Revives fainted Pokemon
    OTHER
};

class Item {
private:
    std::string name;
    ItemType type;
    int quantity;
    int effectValue;  // e.g., HP restored, PP restored

public:
    Item(const std::string& name, ItemType type, int quantity, int effectValue = 0);
    
    // Getters
    std::string getName() const { return name; }
    ItemType getType() const { return type; }
    int getQuantity() const { return quantity; }
    int getEffectValue() const { return effectValue; }
    
    // Use item (decrease quantity)
    bool use();
    
    // Add quantity
    void addQuantity(int amount);
};

#endif // ITEM_H


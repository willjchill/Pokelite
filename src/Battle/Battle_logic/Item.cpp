#include "Item.h"

Item::Item(const std::string& name, ItemType type, int quantity, int effectValue)
    : name(name), type(type), quantity(quantity), effectValue(effectValue) {
}

bool Item::use() {
    if (quantity > 0) {
        quantity--;
        return true;
    }
    return false;
}

void Item::addQuantity(int amount) {
    quantity += amount;
}


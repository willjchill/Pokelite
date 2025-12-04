#include "Bag.h"
#include <algorithm>

Bag::Bag() {
    // Initialize with some default items
    items.push_back(Item("Potion", ItemType::POTION, 3, 20, true));
    items.push_back(Item("Super Potion", ItemType::SUPER_POTION, 1, 50, true));
    // Poke Balls should not be usable in PvP battles
    items.push_back(Item("Poke Ball", ItemType::POKE_BALL, 5, 0, false));
}

void Bag::addItem(const Item& item) {
    // Check if item already exists
    for (auto& existingItem : items) {
        if (existingItem.getName() == item.getName()) {
            existingItem.addQuantity(item.getQuantity());
            return;
        }
    }
    // Add new item
    items.push_back(item);
}

void Bag::removeItem(const std::string& itemName) {
    items.erase(
        std::remove_if(items.begin(), items.end(),
            [&itemName](const Item& item) {
                return item.getName() == itemName && item.getQuantity() == 0;
            }),
        items.end()
    );
}

Item* Bag::getItem(const std::string& itemName) {
    for (auto& item : items) {
        if (item.getName() == itemName) {
            return &item;
        }
    }
    return nullptr;
}

bool Bag::hasItem(const std::string& itemName) const {
    for (const auto& item : items) {
        if (item.getName() == itemName && item.getQuantity() > 0) {
            return true;
        }
    }
    return false;
}


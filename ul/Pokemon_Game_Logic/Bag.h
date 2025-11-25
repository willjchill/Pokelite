#ifndef BAG_H
#define BAG_H

#include "Item.h"
#include <vector>

class Bag {
private:
    std::vector<Item> items;

public:
    Bag();
    
    // Item management
    void addItem(const Item& item);
    void removeItem(const std::string& itemName);
    Item* getItem(const std::string& itemName);
    std::vector<Item>& getItems() { return items; }
    const std::vector<Item>& getItems() const { return items; }
    
    // Check if item exists
    bool hasItem(const std::string& itemName) const;
};

#endif // BAG_H


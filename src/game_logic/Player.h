#ifndef PLAYER_H
#define PLAYER_H

#include "Pokemon.h"
#include "Bag.h"
#include <string>
#include <vector>

enum class PlayerType {
    HUMAN,
    NPC
};

class Player {
private:
    std::string name;
    std::vector<Pokemon> team;
    Bag bag;
    PlayerType playerType;
    int activePokemonIndex;  // Index of currently active Pokemon (0-5)

public:
    Player(const std::string& name, PlayerType type = PlayerType::HUMAN);
    
    // Getters
    std::string getName() const { return name; }
    std::vector<Pokemon>& getTeam() { return team; }
    const std::vector<Pokemon>& getTeam() const { return team; }
    Bag& getBag() { return bag; }
    const Bag& getBag() const { return bag; }
    PlayerType getPlayerType() const { return playerType; }
    int getActivePokemonIndex() const { return activePokemonIndex; }
    
    // Get active Pokemon
    Pokemon* getActivePokemon();
    const Pokemon* getActivePokemon() const;
    
    // Team management
    void addPokemon(const Pokemon& pokemon);
    void switchPokemon(int index);
    bool hasUsablePokemon() const;
    std::vector<int> getUsablePokemonIndices() const;
    
    // Check if player has any Pokemon left
    bool isDefeated() const;
};

#endif // PLAYER_H


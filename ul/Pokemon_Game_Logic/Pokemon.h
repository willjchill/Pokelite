#ifndef POKEMON_H
#define POKEMON_H

#include "Type.h"
#include "Attack.h"
#include "PokemonData.h"
#include <string>
#include <vector>

struct Stats {
    int attack;
    int defense;
    int specialAttack;
    int specialDefense;
    int speed;
    int hp;
};

class Pokemon {
private:
    int dexNumber;  // Species ID from Pokedex
    std::string name;
    int level;
    int currentHP;
    int maxHP;
    int experience;
    int experienceToNextLevel;
    Type primaryType;
    Type secondaryType;
    std::vector<Attack> moves;
    Stats stats;
    bool fainted;
    PokemonSpeciesData speciesData;  // Base stats and species info

public:
    // Create Pokemon by species ID (dex number) and level
    Pokemon(int dexNumber, int level);
    
    // Legacy constructor for backwards compatibility (creates by name lookup)
    Pokemon(const std::string& name, int level, Type primaryType, Type secondaryType = Type::NONE);
    
    // Getters
    int getDexNumber() const { return dexNumber; }
    std::string getName() const { return name; }
    int getLevel() const { return level; }
    int getCurrentHP() const { return currentHP; }
    int getMaxHP() const { return maxHP; }
    int getExperience() const { return experience; }
    Type getPrimaryType() const { return primaryType; }
    Type getSecondaryType() const { return secondaryType; }
    Stats getStats() const { return stats; }
    std::vector<Attack>& getMoves() { return moves; }
    const std::vector<Attack>& getMoves() const { return moves; }
    bool isFainted() const { return fainted; }
    
    // Battle methods
    void takeDamage(int damage);
    void heal(int amount);
    void gainExperience(int exp);
    void levelUp();
    
    // Evolution
    bool canEvolve() const;
    int getEvolutionLevel() const;  // Returns level needed to evolve, or 0 if can't evolve
    std::string getEvolutionName() const;  // Returns name of evolution, or empty if can't evolve
    
    // Move management
    void addMove(const Attack& move);
    void learnMovesForLevel(int level);  // Learn moves available at this level
    bool hasUsableMoves() const;
    
    // Calculate stats based on species base stats and level (Gen 3 formula)
    void calculateStats();
    
    // Get attack or special attack stat based on move category
    int getAttackStat(MoveCategory category) const;
    int getDefenseStat(MoveCategory category) const;
};

#endif // POKEMON_H


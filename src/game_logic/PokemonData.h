#ifndef POKEMON_DATA_H
#define POKEMON_DATA_H

#include "Type.h"
#include "Attack.h"
#include <string>
#include <vector>

// Structure to hold Pokemon species data
struct PokemonSpeciesData {
    int dexNumber;
    std::string name;
    Type primaryType;
    Type secondaryType;
    int baseHP;
    int baseAttack;
    int baseDefense;
    int baseSpecialAttack;
    int baseSpecialDefense;
    int baseSpeed;
    std::string spriteDir;  // Directory path to sprite folder (e.g., "sprites/001_bulbasaur" or from JSON)
};

// Structure to hold evolution data
struct EvolutionData {
    std::string evolvesTo;
    std::string condition;  // e.g., "Lvl 16", "Item: Fire Stone", etc.
    int evolutionLevel;     // Parsed level if condition is level-based
};

// Structure to hold level-up move data
struct LevelUpMove {
    int level;
    std::string moveName;  // Move name as it appears in JSON (e.g., "vine-whip")
    Type moveType;
    int power;
    int accuracy;
    int maxPP;
    MoveCategory category;
};

// Structure to hold move metadata from JSON
struct MoveMetadata {
    std::string name;
    Type type;
    int power;      // Can be -1 if null/status move
    int accuracy;   // Can be -1 if null
    int maxPP;
    MoveCategory category;
    std::string description;
};

// Get Pokemon species data by Pokedex number (1-151)
PokemonSpeciesData getPokemonSpeciesData(int dexNumber);

// Get evolution data for a Pokemon by dex number
std::vector<EvolutionData> getPokemonEvolutionData(int dexNumber);

// Get level-up moveset for a Pokemon by Pokedex number
// Returns moves learned at specific levels
std::vector<LevelUpMove> getPokemonLevelUpMoves(int dexNumber);

// Helper function to create Attack from LevelUpMove
Attack createAttackFromLevelUpMove(const LevelUpMove& moveData);

// Get all Pokemon species data (for iteration)
std::vector<PokemonSpeciesData> getAllPokemonSpeciesData();

// Get move metadata by name (move names use hyphens, e.g., "vine-whip")
MoveMetadata getMoveMetadataByName(const std::string& moveName);

// Initialize data from JSON files (call this before using other functions)
bool initializePokemonDataFromJSON();

#endif // POKEMON_DATA_H

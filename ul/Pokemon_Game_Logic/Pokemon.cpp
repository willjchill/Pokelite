#include "Pokemon.h"
#include "PokemonData.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// New constructor: Create Pokemon by species ID (dex number) and level
Pokemon::Pokemon(int dexNumber, int level)
    : dexNumber(dexNumber), level(level), experience(0), fainted(false) {
    // Initialize JSON data if not already done
    initializePokemonDataFromJSON();
    
    // Load species data from JSON
    speciesData = getPokemonSpeciesData(dexNumber);
    name = speciesData.name;
    primaryType = speciesData.primaryType;
    secondaryType = speciesData.secondaryType;
    
    // Calculate stats from base stats
    calculateStats();
    currentHP = maxHP;
    
    // Learn moves available up to current level (from JSON level-up moves)
    learnMovesForLevel(level);
    
    // Ensure Pokemon has at least one move (give tackle if empty)
    if (moves.empty()) {
        MoveMetadata tackleMeta = getMoveMetadataByName("tackle");
        Attack tackle(tackleMeta.name, tackleMeta.type, tackleMeta.power, tackleMeta.accuracy, tackleMeta.maxPP, tackleMeta.category);
        addMove(tackle);
    }
}

// Legacy constructor: Create Pokemon by name (for backwards compatibility)
Pokemon::Pokemon(const std::string& name, int level, Type primaryType, Type secondaryType)
    : name(name), level(level), primaryType(primaryType), secondaryType(secondaryType), 
      experience(0), fainted(false) {
    // Try to find dex number by name
    dexNumber = -1;
    auto allSpecies = getAllPokemonSpeciesData();
    for (const auto& species : allSpecies) {
        if (species.name == name) {
            dexNumber = species.dexNumber;
            speciesData = species;
            break;
        }
    }
    
    // If not found, create default species data
    if (dexNumber == -1) {
        speciesData = {0, name, primaryType, secondaryType, 50, 50, 50, 50, 50, 50};
    }
    
    // Calculate stats
    calculateStats();
    currentHP = maxHP;
    
    // Learn moves available up to current level
    learnMovesForLevel(level);
    
    // Ensure Pokemon has at least one move
    if (moves.empty()) {
        MoveMetadata tackleMeta = getMoveMetadataByName("tackle");
        Attack tackle(tackleMeta.name, tackleMeta.type, tackleMeta.power, tackleMeta.accuracy, tackleMeta.maxPP, tackleMeta.category);
        addMove(tackle);
    }
}

void Pokemon::calculateStats() {
    // Gen 3 stat calculation formula:
    // HP = ((2 * BaseHP + IV + EV/4) * Level / 100) + Level + 10
    // Other stats = ((2 * Base + IV + EV/4) * Level / 100 + 5) * Nature
    
    // For simplicity, assuming IV=15 (average) and EV=0, Nature=1.0
    // This gives us a reasonable approximation
    
    int iv = 15;  // Average IV
    int ev = 0;   // No EVs for simplicity
    
    // HP calculation (special formula)
    stats.hp = ((2 * speciesData.baseHP + iv + ev / 4) * level / 100) + level + 10;
    
    // Other stats
    stats.attack = ((2 * speciesData.baseAttack + iv + ev / 4) * level / 100) + 5;
    stats.defense = ((2 * speciesData.baseDefense + iv + ev / 4) * level / 100) + 5;
    stats.specialAttack = ((2 * speciesData.baseSpecialAttack + iv + ev / 4) * level / 100) + 5;
    stats.specialDefense = ((2 * speciesData.baseSpecialDefense + iv + ev / 4) * level / 100) + 5;
    stats.speed = ((2 * speciesData.baseSpeed + iv + ev / 4) * level / 100) + 5;
    
    maxHP = stats.hp;
    
    // Gen 3 experience curve: Medium Fast (most Pokemon use this)
    // EXP to next level = level^3
    experienceToNextLevel = level * level * level;
}

void Pokemon::takeDamage(int damage) {
    currentHP = std::max(0, currentHP - damage);
    if (currentHP == 0) {
        fainted = true;
    }
}

void Pokemon::heal(int amount) {
    currentHP = std::min(maxHP, currentHP + amount);
    if (currentHP > 0) {
        fainted = false;
    }
}

void Pokemon::gainExperience(int exp) {
    experience += exp;
    if (experience >= experienceToNextLevel) {
        levelUp();
    }
}

void Pokemon::levelUp() {
    int oldLevel = level;
    level++;
    int oldMaxHP = maxHP;
    calculateStats();
    
    // Restore HP proportional to new max HP
    if (oldMaxHP > 0) {
        currentHP = (currentHP * maxHP) / oldMaxHP;
    } else {
        currentHP = maxHP;  // Full HP if was 0
    }
    
    // Check for moves to learn at new level
    learnMovesForLevel(level);
    
    // Check for evolution (evolution happens automatically when level requirement is met)
    // Note: In a real game, player would be prompted to evolve, but for simplicity we auto-evolve
    std::vector<EvolutionData> evolutions = getPokemonEvolutionData(dexNumber);
    for (const auto& evo : evolutions) {
        if (evo.evolutionLevel > 0 && level >= evo.evolutionLevel) {
            // Find the evolved Pokemon's dex number
            auto allSpecies = getAllPokemonSpeciesData();
            for (const auto& species : allSpecies) {
                if (species.name == evo.evolvesTo) {
                    // Evolve this Pokemon
                    dexNumber = species.dexNumber;
                    speciesData = species;
                    name = species.name;
                    primaryType = species.primaryType;
                    secondaryType = species.secondaryType;
                    std::cout << name << " evolved into " << evo.evolvesTo << "!\n";
                    // Recalculate stats with new base stats
                    int oldMaxHP = maxHP;
                    calculateStats();
                    // Restore HP proportionally
                    if (oldMaxHP > 0) {
                        currentHP = (currentHP * maxHP) / oldMaxHP;
                    }
                    break;
                }
            }
            break;  // Only evolve once per level-up
        }
    }
}

void Pokemon::addMove(const Attack& move) {
    // Check if move already exists
    for (const auto& existingMove : moves) {
        if (existingMove.getName() == move.getName()) {
            return;  // Already know this move
        }
    }
    
    // Add move if we have space (max 4 moves)
    if (moves.size() < 4) {
        moves.push_back(move);
    } else {
        // If we have 4 moves, replace the last one (simple approach)
        // In a real game, player would choose which move to forget
        moves.back() = move;
    }
}

void Pokemon::learnMovesForLevel(int targetLevel) {
    // Get all level-up moves for this Pokemon species from JSON
    std::vector<LevelUpMove> levelUpMoves = getPokemonLevelUpMoves(dexNumber);
    
    // Learn all moves that should be learned at or before this level
    for (const auto& levelUpMove : levelUpMoves) {
        if (levelUpMove.level <= targetLevel) {
            // Check if we already know this move
            bool alreadyKnows = false;
            for (const auto& existingMove : moves) {
                if (existingMove.getName() == levelUpMove.moveName) {
                    alreadyKnows = true;
                    break;
                }
            }
            
            // Learn the move if we don't already know it
            if (!alreadyKnows) {
                Attack newMove = createAttackFromLevelUpMove(levelUpMove);
                addMove(newMove);
                // In a real game, player would be prompted, but for simplicity we auto-learn
                if (levelUpMove.level == targetLevel) {
                    std::cout << name << " learned " << levelUpMove.moveName << "!\n";
                }
            }
        }
    }
}

bool Pokemon::canEvolve() const {
    std::vector<EvolutionData> evolutions = getPokemonEvolutionData(dexNumber);
    for (const auto& evo : evolutions) {
        if (evo.evolutionLevel > 0 && level >= evo.evolutionLevel) {
            return true;
        }
    }
    return false;
}

int Pokemon::getEvolutionLevel() const {
    std::vector<EvolutionData> evolutions = getPokemonEvolutionData(dexNumber);
    for (const auto& evo : evolutions) {
        if (evo.evolutionLevel > 0) {
            return evo.evolutionLevel;
        }
    }
    return 0;
}

std::string Pokemon::getEvolutionName() const {
    std::vector<EvolutionData> evolutions = getPokemonEvolutionData(dexNumber);
    for (const auto& evo : evolutions) {
        if (evo.evolutionLevel > 0) {
            return evo.evolvesTo;
        }
    }
    return "";
}

bool Pokemon::hasUsableMoves() const {
    for (const auto& move : moves) {
        if (move.canUse()) {
            return true;
        }
    }
    return false;
}

int Pokemon::getAttackStat(MoveCategory category) const {
    if (category == MoveCategory::PHYSICAL) {
        return stats.attack;
    } else if (category == MoveCategory::SPECIAL) {
        return stats.specialAttack;
    }
    return 0;  // STATUS moves don't use attack stat
}

int Pokemon::getDefenseStat(MoveCategory category) const {
    if (category == MoveCategory::PHYSICAL) {
        return stats.defense;
    } else if (category == MoveCategory::SPECIAL) {
        return stats.specialDefense;
    }
    return 0;
}


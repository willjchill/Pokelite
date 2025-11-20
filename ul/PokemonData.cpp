#include "PokemonData.h"
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <cctype>

// Simple JSON value extraction helpers
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

static std::string unquote(const std::string& str) {
    if (str.length() >= 2 && str[0] == '"' && str[str.length()-1] == '"') {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

static int parseInt(const std::string& str, int defaultValue = 0) {
    try {
        return std::stoi(str);
    } catch (...) {
        return defaultValue;
    }
}

// Database storage
static std::map<int, PokemonSpeciesData> pokemonDatabase;
static std::map<int, std::vector<EvolutionData>> evolutionDatabase;
static std::map<int, std::vector<LevelUpMove>> movesetDatabase;
static std::map<std::string, MoveMetadata> moveMetadataByName;
static bool dataInitialized = false;

// Type conversion
static Type stringToType(const std::string& typeStr) {
    std::string s = typeStr;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    
    if (s == "normal") return Type::NORMAL;
    if (s == "fire") return Type::FIRE;
    if (s == "water") return Type::WATER;
    if (s == "electric") return Type::ELECTRIC;
    if (s == "grass") return Type::GRASS;
    if (s == "ice") return Type::ICE;
    if (s == "fighting") return Type::FIGHTING;
    if (s == "poison") return Type::POISON;
    if (s == "ground") return Type::GROUND;
    if (s == "flying") return Type::FLYING;
    if (s == "psychic") return Type::PSYCHIC;
    if (s == "bug") return Type::BUG;
    if (s == "rock") return Type::ROCK;
    if (s == "ghost") return Type::GHOST;
    if (s == "dragon") return Type::DRAGON;
    if (s == "dark") return Type::DARK;
    if (s == "steel") return Type::STEEL;
    if (s == "fairy") return Type::NORMAL; // Fairy not in our enum, map to NORMAL
    return Type::NORMAL;
}

static MoveCategory stringToMoveCategory(const std::string& cat) {
    std::string s = cat;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    
    if (s == "physical") return MoveCategory::PHYSICAL;
    if (s == "special") return MoveCategory::SPECIAL;
    return MoveCategory::STATUS;
}

// Simple JSON parser for our specific structure
static void parsePokemonJSON(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Find each Pokemon object
    size_t pos = 0;
    while ((pos = content.find("\"id\":", pos)) != std::string::npos) {
        // Extract ID
        size_t idStart = content.find_first_of("0123456789", pos);
        if (idStart == std::string::npos) {
            pos += 5;
            continue;
        }
        size_t idEnd = content.find_first_not_of("0123456789", idStart);
        if (idEnd == std::string::npos) idEnd = content.length();
        int id = parseInt(content.substr(idStart, idEnd - idStart));
        
        // Extract name
        size_t namePos = content.find("\"name\":", pos);
        if (namePos == std::string::npos) {
            pos += 5;
            continue;
        }
        size_t nameStart = content.find("\"", namePos + 7);
        if (nameStart == std::string::npos) {
            pos += 5;
            continue;
        }
        nameStart += 1;
        size_t nameEnd = content.find("\"", nameStart);
        if (nameEnd == std::string::npos) {
            pos += 5;
            continue;
        }
        std::string name = content.substr(nameStart, nameEnd - nameStart);
        
        // Extract types
        size_t typesPos = content.find("\"types\":", pos);
        Type primaryType = Type::NORMAL;
        Type secondaryType = Type::NONE;
        if (typesPos != std::string::npos) {
            size_t arrayStart = content.find("[", typesPos);
            size_t arrayEnd = content.find("]", arrayStart);
            std::string typesStr = content.substr(arrayStart, arrayEnd - arrayStart);
            
            size_t type1Start = typesStr.find("\"") + 1;
            size_t type1End = typesStr.find("\"", type1Start);
            if (type1Start != std::string::npos && type1End != std::string::npos) {
                primaryType = stringToType(typesStr.substr(type1Start, type1End - type1Start));
            }
            
            size_t type2Start = typesStr.find("\"", type1End + 1);
            size_t type2End = typesStr.find("\"", type2Start + 1);
            if (type2Start != std::string::npos && type2End != std::string::npos) {
                secondaryType = stringToType(typesStr.substr(type2Start + 1, type2End - type2Start - 1));
            }
        }
        
        // Extract base stats
        size_t statsPos = content.find("\"base_stats\":", pos);
        int hp = 50, attack = 50, defense = 50, spAtk = 50, spDef = 50, speed = 50;
        if (statsPos != std::string::npos) {
            // Simple extraction - find each stat value
            auto extractStat = [&](const std::string& statName) -> int {
                size_t statPos = content.find("\"" + statName + "\":", statsPos);
                if (statPos == std::string::npos) return 50;
                size_t valStart = content.find_first_of("0123456789", statPos);
                if (valStart == std::string::npos) return 50;
                size_t valEnd = content.find_first_not_of("0123456789", valStart);
                if (valEnd == std::string::npos) valEnd = content.length();
                return parseInt(content.substr(valStart, valEnd - valStart), 50);
            };
            
            hp = extractStat("hp");
            attack = extractStat("attack");
            defense = extractStat("defense");
            spAtk = extractStat("special-attack");
            spDef = extractStat("special-defense");
            speed = extractStat("speed");
        }
        
        PokemonSpeciesData species = {id, name, primaryType, secondaryType, 
                                      hp, attack, defense, spAtk, spDef, speed};
        pokemonDatabase[id] = species;
        
        // Extract evolution data
        size_t evoPos = content.find("\"evolution\":", pos);
        std::vector<EvolutionData> evolutions;
        if (evoPos != std::string::npos && evoPos + 25 < content.length()) {
            // Check if it's "Final Stage" string or array
            if (content.length() < evoPos + 25 || content.substr(evoPos + 13, 12) != "\"Final Stage\"") {
                // It's an array
                size_t evoArrayStart = content.find("[", evoPos);
                size_t evoArrayEnd = content.find("]", evoArrayStart);
                if (evoArrayStart != std::string::npos && evoArrayEnd != std::string::npos) {
                    size_t evoObjStart = content.find("{", evoArrayStart);
                    while (evoObjStart < evoArrayEnd && evoObjStart != std::string::npos) {
                        size_t evoObjEnd = content.find("}", evoObjStart);
                        
                        // Extract evolves_to
                        size_t toPos = content.find("\"evolves_to\":", evoObjStart);
                        std::string evolvesTo = "";
                        if (toPos != std::string::npos && toPos < evoObjEnd) {
                            size_t toStart = content.find("\"", toPos + 13);
                            if (toStart != std::string::npos && toStart < evoObjEnd) {
                                toStart += 1;
                                size_t toEnd = content.find("\"", toStart);
                                if (toEnd != std::string::npos && toEnd < evoObjEnd) {
                                    evolvesTo = content.substr(toStart, toEnd - toStart);
                                }
                            }
                        }
                        
                        // Extract condition
                        size_t condPos = content.find("\"condition\":", evoObjStart);
                        std::string condition = "";
                        int evoLevel = 0;
                        if (condPos != std::string::npos && condPos < evoObjEnd) {
                            size_t condStart = content.find("\"", condPos + 12);
                            if (condStart != std::string::npos && condStart < evoObjEnd) {
                                condStart += 1;
                                size_t condEnd = content.find("\"", condStart);
                                if (condEnd != std::string::npos && condEnd < evoObjEnd) {
                                    condition = content.substr(condStart, condEnd - condStart);
                                    
                                    // Parse level if condition is "Lvl X"
                                    if (condition.find("Lvl") == 0) {
                                        size_t levelStart = condition.find_first_of("0123456789");
                                        if (levelStart != std::string::npos) {
                                            evoLevel = parseInt(condition.substr(levelStart));
                                        }
                                    }
                                }
                            }
                        }
                        
                        if (!evolvesTo.empty()) {
                            evolutions.push_back({evolvesTo, condition, evoLevel});
                        }
                        
                        evoObjStart = content.find("{", evoObjEnd);
                    }
                }
            }
        }
        evolutionDatabase[id] = evolutions;
        
        // Extract level-up moves
        size_t movesPos = content.find("\"level_up_moves\":", pos);
        std::vector<LevelUpMove> levelUpMoves;
        if (movesPos != std::string::npos) {
            size_t movesArrayStart = content.find("[", movesPos);
            size_t movesArrayEnd = content.find("]", movesArrayStart);
            if (movesArrayStart != std::string::npos && movesArrayEnd != std::string::npos) {
                size_t moveObjStart = content.find("{", movesArrayStart);
                while (moveObjStart < movesArrayEnd && moveObjStart != std::string::npos) {
                    size_t moveObjEnd = content.find("}", moveObjStart);
                    
                    // Extract move name
                    size_t moveNamePos = content.find("\"move\":", moveObjStart);
                    std::string moveName = "";
                    if (moveNamePos != std::string::npos && moveNamePos < moveObjEnd) {
                        size_t nameStart = content.find("\"", moveNamePos + 7);
                        if (nameStart != std::string::npos && nameStart < moveObjEnd) {
                            nameStart += 1;
                            size_t nameEnd = content.find("\"", nameStart);
                            if (nameEnd != std::string::npos && nameEnd < moveObjEnd) {
                                moveName = content.substr(nameStart, nameEnd - nameStart);
                            }
                        }
                    }
                    
                    // Extract level
                    size_t levelPos = content.find("\"level\":", moveObjStart);
                    int level = 1;
                    if (levelPos != std::string::npos && levelPos < moveObjEnd) {
                        size_t levelStart = content.find_first_of("0123456789", levelPos);
                        if (levelStart != std::string::npos && levelStart < moveObjEnd) {
                            size_t levelEnd = content.find_first_not_of("0123456789", levelStart);
                            if (levelEnd == std::string::npos) levelEnd = content.length();
                            level = parseInt(content.substr(levelStart, levelEnd - levelStart), 1);
                        }
                    }
                    
                    if (!moveName.empty()) {
                        // Get move metadata directly from the map (don't call getMoveMetadataByName
                        // which would check initialization - we're still in the middle of parsing)
                        MoveMetadata moveMeta;
                        auto moveIt = moveMetadataByName.find(moveName);
                        if (moveIt != moveMetadataByName.end()) {
                            moveMeta = moveIt->second;
                        } else {
                            // Default move if not found (shouldn't happen if moves were parsed first)
                            moveMeta = {"tackle", Type::NORMAL, 40, 100, 35, MoveCategory::PHYSICAL, ""};
                        }
                        LevelUpMove lum;
                        lum.level = level;
                        lum.moveName = moveName;
                        lum.moveType = moveMeta.type;
                        lum.power = moveMeta.power;
                        lum.accuracy = moveMeta.accuracy;
                        lum.maxPP = moveMeta.maxPP;
                        lum.category = moveMeta.category;
                        levelUpMoves.push_back(lum);
                    }
                    
                    moveObjStart = content.find("{", moveObjEnd);
                }
            }
        }
        movesetDatabase[id] = levelUpMoves;
        
        // Move to next Pokemon
        pos = content.find("\"id\":", pos + 1);
    }
}

static void parseMovesJSON(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Find each move object
    size_t pos = 0;
    while ((pos = content.find("\"", pos)) != std::string::npos) {
        // Extract move key (name)
        size_t keyStart = pos + 1;
        size_t keyEnd = content.find("\"", keyStart);
        if (keyEnd == std::string::npos) break;
        std::string moveKey = content.substr(keyStart, keyEnd - keyStart);
        
        // Find the move object
        size_t objStart = content.find("{", keyEnd);
        if (objStart == std::string::npos) break;
        size_t objEnd = content.find("}", objStart);
        if (objEnd == std::string::npos) break;
        
        // Extract move data
        auto extractString = [&](const std::string& field) -> std::string {
            size_t fieldPos = content.find("\"" + field + "\":", objStart);
            if (fieldPos == std::string::npos || fieldPos > objEnd) return "";
            size_t valStart = content.find("\"", fieldPos + field.length() + 2);
            if (valStart == std::string::npos || valStart > objEnd) return "";
            valStart += 1;
            size_t valEnd = content.find("\"", valStart);
            if (valEnd == std::string::npos || valEnd > objEnd) return "";
            return content.substr(valStart, valEnd - valStart);
        };
        
        auto extractInt = [&](const std::string& field, int defaultValue = -1) -> int {
            size_t fieldPos = content.find("\"" + field + "\":", objStart);
            if (fieldPos == std::string::npos || fieldPos > objEnd) return defaultValue;
            // Check for null
            if (fieldPos + field.length() + 6 < content.length() && 
                content.substr(fieldPos + field.length() + 2, 4) == "null") {
                return defaultValue;
            }
            size_t valStart = content.find_first_of("0123456789-", fieldPos);
            if (valStart == std::string::npos || valStart > objEnd) return defaultValue;
            size_t valEnd = content.find_first_not_of("0123456789", valStart);
            if (valEnd == std::string::npos) valEnd = content.length();
            return parseInt(content.substr(valStart, valEnd - valStart), defaultValue);
        };
        
        std::string name = extractString("name");
        std::string typeStr = extractString("type");
        int power = extractInt("power", 0);
        int accuracy = extractInt("accuracy", 100);
        int pp = extractInt("pp", 20);
        std::string damageClass = extractString("damage_class");
        std::string description = extractString("description");
        
        MoveMetadata meta;
        meta.name = name.empty() ? moveKey : name;
        meta.type = stringToType(typeStr);
        meta.power = power;
        meta.accuracy = accuracy < 0 ? 100 : accuracy;
        meta.maxPP = pp;
        meta.category = stringToMoveCategory(damageClass);
        meta.description = description;
        
        moveMetadataByName[moveKey] = meta;
        
        pos = objEnd + 1;
    }
}

bool initializePokemonDataFromJSON() {
    // Early return if already initialized - this check happens first
    // This is the critical check that ensures parsing only happens once
    if (dataInitialized) {
        return true;
    }
    
    // Parse moves first (needed for level-up moves)
    parseMovesJSON("firered_moves.json");
    
    // Parse Pokemon data
    parsePokemonJSON("firered_full_pokedex.json");
    
    // Set flag immediately after parsing to prevent any race conditions
    // This ensures that even if multiple threads call this, parsing only happens once
    dataInitialized = true;
    return true;
}

PokemonSpeciesData getPokemonSpeciesData(int dexNumber) {
    // Only initialize if not already done (early return in initializePokemonDataFromJSON)
    if (!dataInitialized) {
        initializePokemonDataFromJSON();
    }
    auto it = pokemonDatabase.find(dexNumber);
    if (it != pokemonDatabase.end()) {
        return it->second;
    }
    return {dexNumber, "Unknown", Type::NORMAL, Type::NONE, 50, 50, 50, 50, 50, 50};
}

std::vector<EvolutionData> getPokemonEvolutionData(int dexNumber) {
    // Only initialize if not already done
    if (!dataInitialized) {
        initializePokemonDataFromJSON();
    }
    auto it = evolutionDatabase.find(dexNumber);
    if (it != evolutionDatabase.end()) {
        return it->second;
    }
    return {};
}

std::vector<LevelUpMove> getPokemonLevelUpMoves(int dexNumber) {
    // Only initialize if not already done
    if (!dataInitialized) {
        initializePokemonDataFromJSON();
    }
    auto it = movesetDatabase.find(dexNumber);
    if (it != movesetDatabase.end()) {
        return it->second;
    }
    return {};
}

Attack createAttackFromLevelUpMove(const LevelUpMove& moveData) {
    return Attack(moveData.moveName, moveData.moveType, moveData.power,
                  moveData.accuracy, moveData.maxPP, moveData.category);
}

std::vector<PokemonSpeciesData> getAllPokemonSpeciesData() {
    // Only initialize if not already done
    if (!dataInitialized) {
        initializePokemonDataFromJSON();
    }
    std::vector<PokemonSpeciesData> allPokemon;
    for (const auto& entry : pokemonDatabase) {
        allPokemon.push_back(entry.second);
    }
    return allPokemon;
}

MoveMetadata getMoveMetadataByName(const std::string& moveName) {
    // Only initialize if not already done
    if (!dataInitialized) {
        initializePokemonDataFromJSON();
    }
    auto it = moveMetadataByName.find(moveName);
    if (it != moveMetadataByName.end()) {
        return it->second;
    }
    // Return default move if not found
    return {"tackle", Type::NORMAL, 40, 100, 35, MoveCategory::PHYSICAL, ""};
}

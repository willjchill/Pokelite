#include "PokemonData.h"
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <cctype>
#include  "json/json.h"

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

// JSON parser using jsoncpp library
static void parsePokemonJSON(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errors;
    
    if (!Json::parseFromStream(readerBuilder, file, &root, &errors)) {
        file.close();
        return;
    }
    
    file.close();
    
    // Iterate through array of Pokemon
    if (!root.isArray()) {
        return;
    }
    
    for (const auto& pokemon : root) {
        if (!pokemon.isObject()) continue;
        
        // Extract ID
        int id = pokemon.get("id", 0).asInt();
        if (id == 0) continue;
        
        // Extract name
        std::string name = pokemon.get("name", "").asString();
        if (name.empty()) continue;
        
        // Extract types
        Type primaryType = Type::NORMAL;
        Type secondaryType = Type::NONE;
        if (pokemon.isMember("types") && pokemon["types"].isArray()) {
            const Json::Value& types = pokemon["types"];
            if (types.size() > 0 && types[0].isString()) {
                primaryType = stringToType(types[0].asString());
            }
            if (types.size() > 1 && types[1].isString()) {
                secondaryType = stringToType(types[1].asString());
            }
        }
        
        // Extract base stats
        int hp = 50, attack = 50, defense = 50, spAtk = 50, spDef = 50, speed = 50;
        if (pokemon.isMember("base_stats") && pokemon["base_stats"].isObject()) {
            const Json::Value& stats = pokemon["base_stats"];
            hp = stats.get("hp", 50).asInt();
            attack = stats.get("attack", 50).asInt();
            defense = stats.get("defense", 50).asInt();
            spAtk = stats.get("special-attack", 50).asInt();
            spDef = stats.get("special-defense", 50).asInt();
            speed = stats.get("speed", 50).asInt();
        }
        
        PokemonSpeciesData species = {id, name, primaryType, secondaryType, 
                                      hp, attack, defense, spAtk, spDef, speed};
        pokemonDatabase[id] = species;
        
        // Extract evolution data
        std::vector<EvolutionData> evolutions;
        if (pokemon.isMember("evolution")) {
            const Json::Value& evo = pokemon["evolution"];
            if (evo.isArray()) {
                for (const auto& evoItem : evo) {
                    if (evoItem.isObject()) {
                        std::string evolvesTo = evoItem.get("evolves_to", "").asString();
                        std::string condition = evoItem.get("condition", "").asString();
                        int evoLevel = 0;
                        
                        // Parse level if condition is "Lvl X"
                        if (condition.find("Lvl") == 0) {
                            size_t levelStart = condition.find_first_of("0123456789");
                            if (levelStart != std::string::npos) {
                                try {
                                    evoLevel = std::stoi(condition.substr(levelStart));
                                } catch (...) {
                                    evoLevel = 0;
                                }
                            }
                        }
                        
                        if (!evolvesTo.empty()) {
                            evolutions.push_back({evolvesTo, condition, evoLevel});
                        }
                    }
                }
            }
        }
        evolutionDatabase[id] = evolutions;
        
        // Extract level-up moves
        std::vector<LevelUpMove> levelUpMoves;
        if (pokemon.isMember("level_up_moves") && pokemon["level_up_moves"].isArray()) {
            const Json::Value& moves = pokemon["level_up_moves"];
            for (const auto& moveItem : moves) {
                if (moveItem.isObject()) {
                    std::string moveName = moveItem.get("move", "").asString();
                    int level = moveItem.get("level", 1).asInt();
                    
                    if (!moveName.empty()) {
                        // Get move metadata directly from the map
                        MoveMetadata moveMeta;
                        auto moveIt = moveMetadataByName.find(moveName);
                        if (moveIt != moveMetadataByName.end()) {
                            moveMeta = moveIt->second;
                        } else {
                            // Default move if not found
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
                }
            }
        }
        movesetDatabase[id] = levelUpMoves;
    }
}

static void parseMovesJSON(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errors;
    
    if (!Json::parseFromStream(readerBuilder, file, &root, &errors)) {
        file.close();
        return;
    }
    
    file.close();
    
    // Iterate through object (keys are move names)
    if (!root.isObject()) {
        return;
    }
    
    for (auto it = root.begin(); it != root.end(); ++it) {
        std::string moveKey = it.key().asString();
        const Json::Value& moveObj = *it;
        
        if (!moveObj.isObject()) continue;
        
        std::string name = moveObj.get("name", moveKey).asString();
        std::string typeStr = moveObj.get("type", "").asString();
        
        // Handle null values for power and accuracy
        int power = 0;
        if (moveObj.isMember("power") && !moveObj["power"].isNull()) {
            power = moveObj["power"].asInt();
        }
        
        int accuracy = 100;
        if (moveObj.isMember("accuracy") && !moveObj["accuracy"].isNull()) {
            accuracy = moveObj["accuracy"].asInt();
            if (accuracy < 0) accuracy = 100;
        }
        
        int pp = moveObj.get("pp", 20).asInt();
        std::string damageClass = moveObj.get("damage_class", "").asString();
        std::string description = moveObj.get("description", "").asString();
        
        MoveMetadata meta;
        meta.name = name.empty() ? moveKey : name;
        meta.type = stringToType(typeStr);
        meta.power = power;
        meta.accuracy = accuracy;
        meta.maxPP = pp;
        meta.category = stringToMoveCategory(damageClass);
        meta.description = description;
        
        moveMetadataByName[moveKey] = meta;
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

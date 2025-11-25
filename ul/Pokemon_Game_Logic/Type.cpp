#include "Type.h"
#include <map>
#include <set>

// Type effectiveness chart for Generation 3
double getTypeEffectiveness(Type attackType, Type defenderType1, Type defenderType2) {
    // Map of attack type to set of types it's super effective against
    static std::map<Type, std::set<Type>> superEffective = {
        {Type::NORMAL, {}},
        {Type::FIRE, {Type::GRASS, Type::ICE, Type::BUG, Type::STEEL}},
        {Type::WATER, {Type::FIRE, Type::GROUND, Type::ROCK}},
        {Type::ELECTRIC, {Type::WATER, Type::FLYING}},
        {Type::GRASS, {Type::WATER, Type::GROUND, Type::ROCK}},
        {Type::ICE, {Type::GRASS, Type::GROUND, Type::FLYING, Type::DRAGON}},
        {Type::FIGHTING, {Type::NORMAL, Type::ICE, Type::ROCK, Type::DARK, Type::STEEL}},
        {Type::POISON, {Type::GRASS}},
        {Type::GROUND, {Type::FIRE, Type::ELECTRIC, Type::POISON, Type::ROCK, Type::STEEL}},
        {Type::FLYING, {Type::GRASS, Type::FIGHTING, Type::BUG}},
        {Type::PSYCHIC, {Type::FIGHTING, Type::POISON}},
        {Type::BUG, {Type::GRASS, Type::PSYCHIC, Type::DARK}},
        {Type::ROCK, {Type::FIRE, Type::ICE, Type::FLYING, Type::BUG}},
        {Type::GHOST, {Type::PSYCHIC, Type::GHOST}},
        {Type::DRAGON, {Type::DRAGON}},
        {Type::DARK, {Type::PSYCHIC, Type::GHOST}},
        {Type::STEEL, {Type::ICE, Type::ROCK}}
    };
    
    // Map of attack type to set of types it's not very effective against
    static std::map<Type, std::set<Type>> notVeryEffective = {
        {Type::NORMAL, {Type::ROCK, Type::STEEL}},
        {Type::FIRE, {Type::FIRE, Type::WATER, Type::ROCK, Type::DRAGON}},
        {Type::WATER, {Type::WATER, Type::GRASS, Type::DRAGON}},
        {Type::ELECTRIC, {Type::ELECTRIC, Type::GRASS, Type::DRAGON}},
        {Type::GRASS, {Type::FIRE, Type::GRASS, Type::POISON, Type::FLYING, Type::BUG, Type::DRAGON, Type::STEEL}},
        {Type::ICE, {Type::FIRE, Type::WATER, Type::ICE, Type::STEEL}},
        {Type::FIGHTING, {Type::POISON, Type::FLYING, Type::PSYCHIC, Type::BUG}},
        {Type::POISON, {Type::POISON, Type::GROUND, Type::ROCK, Type::GHOST}},
        {Type::GROUND, {Type::GRASS, Type::BUG}},
        {Type::FLYING, {Type::ELECTRIC, Type::ROCK, Type::STEEL}},
        {Type::PSYCHIC, {Type::PSYCHIC, Type::STEEL}},
        {Type::BUG, {Type::FIRE, Type::FIGHTING, Type::POISON, Type::FLYING, Type::GHOST, Type::STEEL}},
        {Type::ROCK, {Type::FIGHTING, Type::GROUND, Type::STEEL}},
        {Type::GHOST, {Type::DARK}},
        {Type::DRAGON, {Type::STEEL}},
        {Type::DARK, {Type::FIGHTING, Type::DARK, Type::STEEL}},
        {Type::STEEL, {Type::FIRE, Type::WATER, Type::ELECTRIC, Type::STEEL}}
    };
    
    // Map of attack type to set of types it has no effect on
    static std::map<Type, std::set<Type>> noEffect = {
        {Type::NORMAL, {Type::GHOST}},
        {Type::ELECTRIC, {Type::GROUND}},
        {Type::FIGHTING, {Type::GHOST}},
        {Type::POISON, {Type::STEEL}},
        {Type::GROUND, {Type::FLYING}},
        {Type::PSYCHIC, {Type::DARK}},
        {Type::GHOST, {Type::NORMAL}},
        {Type::DRAGON, {}}
    };
    
    double multiplier = 1.0;
    
    // Check first type
    if (noEffect[attackType].count(defenderType1) > 0) {
        return 0.0;
    }
    if (superEffective[attackType].count(defenderType1) > 0) {
        multiplier *= 2.0;
    } else if (notVeryEffective[attackType].count(defenderType1) > 0) {
        multiplier *= 0.5;
    }
    
    // Check second type if dual-type
    if (defenderType2 != Type::NONE) {
        if (noEffect[attackType].count(defenderType2) > 0) {
            return 0.0;
        }
        if (superEffective[attackType].count(defenderType2) > 0) {
            multiplier *= 2.0;
        } else if (notVeryEffective[attackType].count(defenderType2) > 0) {
            multiplier *= 0.5;
        }
    }
    
    return multiplier;
}


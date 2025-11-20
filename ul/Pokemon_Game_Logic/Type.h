#ifndef TYPE_H
#define TYPE_H

enum class Type {
    NORMAL,
    FIRE,
    WATER,
    ELECTRIC,
    GRASS,
    ICE,
    FIGHTING,
    POISON,
    GROUND,
    FLYING,
    PSYCHIC,
    BUG,
    ROCK,
    GHOST,
    DRAGON,
    DARK,
    STEEL,
    NONE  // For single-type Pokemon
};

// Type effectiveness chart for Gen 3
// Returns multiplier: 2.0 = super effective, 0.5 = not very effective, 0.0 = no effect
double getTypeEffectiveness(Type attackType, Type defenderType1, Type defenderType2 = Type::NONE);

#endif // TYPE_H


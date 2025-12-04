#include "Player.h"
#include <algorithm>

Player::Player(const std::string& name, PlayerType type)
    : name(name), playerType(type), activePokemonIndex(0) {
}

Pokemon* Player::getActivePokemon() {
    if (activePokemonIndex >= 0 && activePokemonIndex < static_cast<int>(team.size())) {
        return &team[activePokemonIndex];
    }
    return nullptr;
}

const Pokemon* Player::getActivePokemon() const {
    if (activePokemonIndex >= 0 && activePokemonIndex < static_cast<int>(team.size())) {
        return &team[activePokemonIndex];
    }
    return nullptr;
}

void Player::addPokemon(const Pokemon& pokemon) {
    if (team.size() < 6) {
        team.push_back(pokemon);
    }
}

void Player::switchPokemon(int index) {
    if (index >= 0 && index < static_cast<int>(team.size()) && !team[index].isFainted()) {
        activePokemonIndex = index;
    }
}

bool Player::hasUsablePokemon() const {
    for (const auto& pokemon : team) {
        if (!pokemon.isFainted()) {
            return true;
        }
    }
    return false;
}

std::vector<int> Player::getUsablePokemonIndices() const {
    std::vector<int> indices;
    for (size_t i = 0; i < team.size(); ++i) {
        if (!team[i].isFainted()) {
            indices.push_back(static_cast<int>(i));
        }
    }
    return indices;
}

bool Player::isDefeated() const {
    return !hasUsablePokemon();
}

void Player::removePokemon(int index) {
    if (index >= 0 && index < static_cast<int>(team.size())) {
        team.erase(team.begin() + index);
        // Adjust active Pokemon index if necessary
        if (activePokemonIndex >= static_cast<int>(team.size())) {
            activePokemonIndex = team.empty() ? -1 : static_cast<int>(team.size()) - 1;
        } else if (activePokemonIndex > index) {
            activePokemonIndex--;
        }
        // If the removed Pokemon was active and there are other Pokemon, switch to first available
        if (activePokemonIndex < 0 && !team.empty()) {
            for (size_t i = 0; i < team.size(); ++i) {
                if (!team[i].isFainted()) {
                    activePokemonIndex = static_cast<int>(i);
                    break;
                }
            }
        }
    }
}


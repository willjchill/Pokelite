#include "BattleState_BT.h"
#include <algorithm>

BattleSystem::BattleSystem() : battle(nullptr) {
}

BattleSystem::~BattleSystem() {
}

void BattleSystem::initializeBattle(Player* player1, Player* player2, bool isWild) {
    battle = std::make_unique<Battle>(player1, player2, isWild);
    if (isPvpMode) {
        battle->setPvpMode(true);
    }
}

void BattleSystem::startBattle() {
    if (battle) {
        battle->startBattle();
        lastMessage = "What will " + getPlayerPokemonName() + " do?";
    }
}

void BattleSystem::processAction(BattleAction action) {
    if (battle) {
        battle->processAction(action);
        updateLastMessage();
    }
}

void BattleSystem::processFightAction(int moveIndex) {
    if (battle) {
        // Capture enemy move name before processing (enemy turn happens during processFightAction)
        if (battle->getPlayer2() && battle->getPlayer2()->getActivePokemon()) {
            const Pokemon* enemy = battle->getPlayer2()->getActivePokemon();
            const auto& moves = enemy->getMoves();
            if (!moves.empty()) {
                // Enemy will use a random move, we'll update this after the turn
                // For now, we'll get it from the battle's last message or track it separately
            }
        }
        battle->processFightAction(moveIndex);
        updateLastMessage();
    }
}

void BattleSystem::processBagAction(int itemIndex) {
    if (battle && battle->getPlayer1()) {
        // Get item name before processing
        const auto& items = battle->getPlayer1()->getBag().getItems();
        QString itemName = "item";
        if (itemIndex >= 0 && itemIndex < static_cast<int>(items.size()) && items[itemIndex].getQuantity() > 0) {
            itemName = capitalizeFirst(toQString(items[itemIndex].getName()));
        }

        battle->processBagAction(itemIndex);

        // Set message about item use
        lastMessage = "You used " + itemName + "!";

        // Don't call updateLastMessage() as it would overwrite our custom message
    }
}

void BattleSystem::processPokemonAction(int pokemonIndex) {
    if (battle && battle->getPlayer1()) {
        const auto& team = battle->getPlayer1()->getTeam();
        int activeIndex = battle->getPlayer1()->getActivePokemonIndex();
        
        // Check if trying to switch to the same Pokemon
        if (pokemonIndex == activeIndex) {
            QString pokemonName = capitalizeFirst(toQString(team[pokemonIndex].getName()));
            lastMessage = "You are already using " + pokemonName + "!";
            // Don't process the switch, just set the message
            return;
        }
        
        // Check if Pokemon is fainted
        if (pokemonIndex >= 0 && pokemonIndex < static_cast<int>(team.size()) && team[pokemonIndex].isFainted()) {
            lastMessage = "That Pokemon has fainted!";
            return;
        }
        
        // Get Pokemon name before switching
        QString pokemonName = "Pokemon";
        if (pokemonIndex >= 0 && pokemonIndex < static_cast<int>(team.size())) {
            pokemonName = capitalizeFirst(toQString(team[pokemonIndex].getName()));
        }
        
        battle->processPokemonAction(pokemonIndex);
        
        // Set message about switching
        lastMessage = "Switched to " + pokemonName + "!";
        
        // Don't call updateLastMessage() as it would overwrite our custom message
    }
}

void BattleSystem::processRunAction() {
    if (battle) {
        battle->processRunAction();
        // Set appropriate message based on result
        if (battle->getState() == BattleState::BATTLE_END) {
            lastMessage = "Got away safely!";
        } else {
            lastMessage = "Can't escape!";
        }
    }
}

void BattleSystem::returnToMainMenu() {
    if (battle) {
        battle->returnToMainMenu();
        updateLastMessage();
    }
}

BattleState BattleSystem::getState() const {
    if (battle) {
        return battle->getState();
    }
    return BattleState::BATTLE_END;
}

bool BattleSystem::isBattleOver() const {
    if (battle) {
        return battle->isBattleOver();
    }
    return true;
}

Player* BattleSystem::getWinner() const {
    if (battle) {
        return battle->getWinner();
    }
    return nullptr;
}

QString BattleSystem::getPlayerPokemonName() const {
    if (battle && battle->getPlayer1()) {
        const Pokemon* pokemon = battle->getPlayer1()->getActivePokemon();
        if (pokemon) {
            return toQString(pokemon->getName());
        }
    }
    return "";
}

QString BattleSystem::getEnemyPokemonName() const {
    if (battle && battle->getPlayer2()) {
        const Pokemon* pokemon = battle->getPlayer2()->getActivePokemon();
        if (pokemon) {
            return toQString(pokemon->getName());
        }
    }
    return "";
}

int BattleSystem::getPlayerHP() const {
    if (battle && battle->getPlayer1()) {
        const Pokemon* pokemon = battle->getPlayer1()->getActivePokemon();
        if (pokemon) {
            return pokemon->getCurrentHP();
        }
    }
    return 0;
}

int BattleSystem::getPlayerMaxHP() const {
    if (battle && battle->getPlayer1()) {
        const Pokemon* pokemon = battle->getPlayer1()->getActivePokemon();
        if (pokemon) {
            return pokemon->getMaxHP();
        }
    }
    return 0;
}

int BattleSystem::getEnemyHP() const {
    if (battle && battle->getPlayer2()) {
        const Pokemon* pokemon = battle->getPlayer2()->getActivePokemon();
        if (pokemon) {
            return pokemon->getCurrentHP();
        }
    }
    return 0;
}

int BattleSystem::getEnemyMaxHP() const {
    if (battle && battle->getPlayer2()) {
        const Pokemon* pokemon = battle->getPlayer2()->getActivePokemon();
        if (pokemon) {
            return pokemon->getMaxHP();
        }
    }
    return 0;
}

std::vector<QString> BattleSystem::getPlayerMoves() const {
    std::vector<QString> moves;
    if (battle && battle->getPlayer1()) {
        const Pokemon* pokemon = battle->getPlayer1()->getActivePokemon();
        if (pokemon) {
            const auto& moveList = pokemon->getMoves();
            for (const auto& move : moveList) {
                moves.push_back(toQString(move.getName()));
            }
        }
    }
    return moves;
}

std::vector<int> BattleSystem::getPlayerMovePP() const {
    std::vector<int> pp;
    if (battle && battle->getPlayer1()) {
        const Pokemon* pokemon = battle->getPlayer1()->getActivePokemon();
        if (pokemon) {
            const auto& moveList = pokemon->getMoves();
            for (const auto& move : moveList) {
                pp.push_back(move.getCurrentPP());
            }
        }
    }
    return pp;
}

std::vector<int> BattleSystem::getPlayerMoveMaxPP() const {
    std::vector<int> maxPP;
    if (battle && battle->getPlayer1()) {
        const Pokemon* pokemon = battle->getPlayer1()->getActivePokemon();
        if (pokemon) {
            const auto& moveList = pokemon->getMoves();
            for (const auto& move : moveList) {
                maxPP.push_back(move.getMaxPP());
            }
        }
    }
    return maxPP;
}

std::vector<QString> BattleSystem::getBagItems() const {
    std::vector<QString> items;
    if (battle && battle->getPlayer1()) {
        const auto& itemList = battle->getPlayer1()->getBag().getItems();
        for (const auto& item : itemList) {
            if (item.getQuantity() > 0) {
                items.push_back(toQString(item.getName()));
            }
        }
    }
    return items;
}

std::vector<int> BattleSystem::getBagItemQuantities() const {
    std::vector<int> quantities;
    if (battle && battle->getPlayer1()) {
        const auto& itemList = battle->getPlayer1()->getBag().getItems();
        for (const auto& item : itemList) {
            if (item.getQuantity() > 0) {
                quantities.push_back(item.getQuantity());
            }
        }
    }
    return quantities;
}

std::vector<QString> BattleSystem::getTeamNames() const {
    std::vector<QString> names;
    if (battle && battle->getPlayer1()) {
        const auto& team = battle->getPlayer1()->getTeam();
        for (const auto& pokemon : team) {
            names.push_back(toQString(pokemon.getName()));
        }
    }
    return names;
}

std::vector<int> BattleSystem::getTeamHP() const {
    std::vector<int> hp;
    if (battle && battle->getPlayer1()) {
        const auto& team = battle->getPlayer1()->getTeam();
        for (const auto& pokemon : team) {
            hp.push_back(pokemon.getCurrentHP());
        }
    }
    return hp;
}

std::vector<int> BattleSystem::getTeamMaxHP() const {
    std::vector<int> maxHP;
    if (battle && battle->getPlayer1()) {
        const auto& team = battle->getPlayer1()->getTeam();
        for (const auto& pokemon : team) {
            maxHP.push_back(pokemon.getMaxHP());
        }
    }
    return maxHP;
}

std::vector<bool> BattleSystem::getTeamFainted() const {
    std::vector<bool> fainted;
    if (battle && battle->getPlayer1()) {
        const auto& team = battle->getPlayer1()->getTeam();
        for (const auto& pokemon : team) {
            fainted.push_back(pokemon.isFainted());
        }
    }
    return fainted;
}

std::vector<int> BattleSystem::getTeamLevels() const {
    std::vector<int> levels;
    if (battle && battle->getPlayer1()) {
        const auto& team = battle->getPlayer1()->getTeam();
        for (const auto& pokemon : team) {
            levels.push_back(pokemon.getLevel());
        }
    }
    return levels;
}

int BattleSystem::getActivePokemonIndex() const {
    if (battle && battle->getPlayer1()) {
        return battle->getPlayer1()->getActivePokemonIndex();
    }
    return 0;
}

bool BattleSystem::isWaitingForPlayerMove() const {
    if (battle) {
        return battle->getState() == BattleState::FIGHT_MENU;
    }
    return false;
}

bool BattleSystem::isWaitingForEnemyTurn() const {
    if (battle) {
        return battle->getState() == BattleState::EXECUTING_TURN;
    }
    return false;
}

QString BattleSystem::toQString(const std::string& str) const {
    return QString::fromStdString(str);
}

QString BattleSystem::capitalizeFirst(const QString& str) const {
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1).toLower();
}

QString BattleSystem::getEnemyLastMoveName() const {
    if (battle) {
        return toQString(battle->getLastEnemyMoveName());
    }
    return "";
}

void BattleSystem::updateLastMessage() {
    // Update last message based on battle state
    if (!battle) return;
    
    BattleState state = battle->getState();
    switch (state) {
        case BattleState::MENU:
            lastMessage = "What will " + getPlayerPokemonName() + " do?";
            break;
        case BattleState::FIGHT_MENU:
            lastMessage = "Choose a move!";
            break;
        case BattleState::BAG_MENU:
            lastMessage = "Choose an item!";
            break;
        case BattleState::POKEMON_MENU:
            lastMessage = "Choose a Pokemon!";
            break;
        case BattleState::RUN_CONFIRM:
            // Message will be set by processRunAction
            break;
        default:
            break;
    }
}

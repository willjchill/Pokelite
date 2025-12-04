#include "Battle.h"
#include "Attack.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

Battle::Battle(Player* p1, Player* p2, bool isWild)
    : player1(p1), player2(p2), isWildBattle(isWild), isPvpMode(false), escapeAttempts(0),
      state(BattleState::SETUP), gen(rd()),
      damageRandom(85, 100), accuracyCheck(1, 100), runCheck(0, 255), criticalCheck(1, 100) {
}

void Battle::startBattle() {
    state = BattleState::SETUP;
    
    // Round Setup
    std::cout << "\n=== BATTLE START ===\n";
    std::cout << "[" << player2->getName() << "] would like to battle!\n";
    std::cout << "\nGo! " << player1->getActivePokemon()->getName() << "!\n";
    
    if (!isWildBattle) {
        std::cout << "Go! " << player2->getActivePokemon()->getName() << "!\n";
    }
    
    displayBattleStatus();
    state = BattleState::MENU;
}

void Battle::displayBattleStatus() const {
    const Pokemon* p1Pokemon = player1->getActivePokemon();
    const Pokemon* p2Pokemon = player2->getActivePokemon();
    
    if (p1Pokemon && p2Pokemon) {
        std::cout << "\n--- Battle Status ---\n";
        std::cout << "Player: " << p1Pokemon->getName() 
                  << ", LVL " << p1Pokemon->getLevel()
                  << ", HP " << p1Pokemon->getCurrentHP() << "/" << p1Pokemon->getMaxHP() << "\n";
        std::cout << "Opponent: " << p2Pokemon->getName() 
                  << ", LVL " << p2Pokemon->getLevel()
                  << ", HP " << p2Pokemon->getCurrentHP() << "/" << p2Pokemon->getMaxHP() << "\n";
        std::cout << "-------------------\n";
    }
}

void Battle::displayMainMenu() const {
    const Pokemon* active = player1->getActivePokemon();
    if (!active) return;
    
    std::cout << "\nWhat will " << active->getName() << " do?\n";
    std::cout << "[FIGHT] [BAG]\n";
    std::cout << "[POKEMON] [RUN]\n";
}

void Battle::displayFightMenu() const {
    const Pokemon* active = player1->getActivePokemon();
    if (!active) return;
    
    const auto& moves = active->getMoves();
    std::cout << "\n--- Select Move ---\n";
    
    for (size_t i = 0; i < moves.size(); ++i) {
        std::cout << "[" << (i + 1) << "] " << moves[i].getName() 
                  << " | PP " << moves[i].getCurrentPP() << "/" << moves[i].getMaxPP();
        
        if (i % 2 == 0 && i + 1 < moves.size()) {
            std::cout << "\t";
        } else {
            std::cout << "\n";
        }
    }
    
    // Display type info (simplified - showing type enum values)
    std::cout << "Type: ";
    if (active->getSecondaryType() != Type::NONE) {
        std::cout << "[Type " << static_cast<int>(active->getPrimaryType()) << "/" 
                  << static_cast<int>(active->getSecondaryType()) << "]\n";
    } else {
        std::cout << "[Type " << static_cast<int>(active->getPrimaryType()) << "]\n";
    }
}

void Battle::displayBagMenu() const {
    const auto& items = player1->getBag().getItems();
    std::cout << "\n--- Bag ---\n";
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].getQuantity() > 0) {
            std::cout << "[" << (i + 1) << "] " << items[i].getName() 
                      << " x" << items[i].getQuantity() << "\n";
        }
    }
}

void Battle::displayPokemonMenu() const {
    const auto& team = player1->getTeam();
    std::cout << "\n--- Pokemon ---\n";
    for (size_t i = 0; i < team.size(); ++i) {
        std::cout << "[" << (i + 1) << "] " << team[i].getName();
        if (team[i].isFainted()) {
            std::cout << " (FAINTED)";
        } else if (static_cast<int>(i) == player1->getActivePokemonIndex()) {
            std::cout << " (ACTIVE)";
        }
        std::cout << " | HP " << team[i].getCurrentHP() << "/" << team[i].getMaxHP() << "\n";
    }
    std::cout << "[SEND OUT] [SUMMARY] [CANCEL]\n";
}

int Battle::calculateDamage(Pokemon& attacker, Pokemon& defender, Attack& move) {
    if (move.getCategory() == MoveCategory::STATUS) {
        return 0;  // Status moves don't deal damage
    }
    
    // Gen 3 Damage Formula:
    // Damage = (((2 * Level / 5 + 2) * BasePower * (Attack / Defense)) / 50 + 2) * Modifier
    
    int level = attacker.getLevel();
    int basePower = move.getPower();
    int attackStat = attacker.getAttackStat(move.getCategory());
    int defenseStat = defender.getDefenseStat(move.getCategory());
    
    // Base damage calculation
    int damage = ((2 * level / 5 + 2) * basePower * attackStat / defenseStat) / 50 + 2;
    
    // Modifier calculation
    double modifier = 1.0;
    
    // 1. STAB (Same-Type Attack Bonus) - 1.5x if move type matches Pokemon type
    if (move.getType() == attacker.getPrimaryType() || 
        move.getType() == attacker.getSecondaryType()) {
        modifier *= 1.5;
    }
    
    // 2. Type effectiveness
    double typeEffectiveness = getTypeEffectiveness(
        move.getType(), 
        defender.getPrimaryType(), 
        defender.getSecondaryType()
    );
    modifier *= typeEffectiveness;
    
    // 3. Critical hit (Gen 3: 1/16 chance for most moves, 2x damage)
    bool isCritical = checkCriticalHit();
    if (isCritical) {
        modifier *= 2.0;
        std::cout << "Critical hit!\n";
    }
    
    // 4. Random factor (85-100%)
    int randomFactor = damageRandom(gen);
    modifier *= (randomFactor / 100.0);
    
    // Apply modifier
    damage = static_cast<int>(damage * modifier);
    
    // Type effectiveness messages
    if (typeEffectiveness > 1.0) {
        std::cout << "It's super effective!\n";
    } else if (typeEffectiveness > 0.0 && typeEffectiveness < 1.0) {
        std::cout << "It's not very effective...\n";
    } else if (typeEffectiveness == 0.0) {
        std::cout << "It doesn't affect " << defender.getName() << "!\n";
        return 0;
    }
    
    return std::max(1, damage);  // Minimum 1 damage
}

bool Battle::checkAccuracy(const Attack& move) {
    // In Gen 3, accuracy is checked against a random number 1-100
    // If random number <= accuracy, move hits
    int roll = accuracyCheck(gen);
    return roll <= move.getAccuracy();
}

bool Battle::checkCriticalHit() {
    // Gen 3: 1/16 chance (6.25%) for critical hit
    int roll = criticalCheck(gen);
    return roll <= 6;  // 6 out of 100 = ~6% (close to 1/16)
}

bool Battle::attemptRun() {
    if (!isWildBattle) {
        // Cannot run from trainer battles
        std::cout << "You can't run from a trainer battle!\n";
        return false;
    }
    
    // Gen 3 Run Formula:
    // A = ((PlayerSpeed * 32) / EnemySpeed) + (EscapeAttempts * 30)
    // If A >= random(0-255), escape succeeds
    
    const Pokemon* playerPokemon = player1->getActivePokemon();
    const Pokemon* enemyPokemon = player2->getActivePokemon();
    
    if (!playerPokemon || !enemyPokemon) {
        return false;
    }
    
    int playerSpeed = playerPokemon->getStats().speed;
    int enemySpeed = enemyPokemon->getStats().speed;
    
    int A = ((playerSpeed * 32) / std::max(1, enemySpeed)) + (escapeAttempts * 30);
    int randomValue = runCheck(gen);
    
    escapeAttempts++;
    
    if (A >= randomValue) {
        std::cout << "Got away safely!\n";
        return true;
    } else {
        std::cout << "Can't escape!\n";
        return false;
    }
}

int Battle::determineTurnOrder() {
    const Pokemon* p1Pokemon = player1->getActivePokemon();
    const Pokemon* p2Pokemon = player2->getActivePokemon();
    
    if (!p1Pokemon || !p2Pokemon) {
        return 1;  // Default to player1 first
    }
    
    int p1Speed = p1Pokemon->getStats().speed;
    int p2Speed = p2Pokemon->getStats().speed;
    
    // If speeds are equal, random
    if (p1Speed == p2Speed) {
        std::uniform_int_distribution<> coinFlip(0, 1);
        return coinFlip(gen);
    }
    
    return (p1Speed > p2Speed) ? 1 : 2;
}

void Battle::awardExperience(Pokemon& winner, Pokemon& loser) {
    // Gen 3 Experience Formula (simplified):
    // EXP = (Base EXP * Level * Trainer modifier) / 7
    // Base EXP varies by species, but we'll use a simplified formula:
    // EXP = (loser's level * baseExpMultiplier)
    
    // Base experience multiplier (varies by species, using average)
    int baseExpMultiplier = 60;  // Average base EXP for most Pokemon
    
    // Calculate experience gained
    int expGained = (loser.getLevel() * baseExpMultiplier) / 7;
    
    // Minimum 1 EXP
    if (expGained < 1) expGained = 1;
    
    std::cout << winner.getName() << " gained " << expGained << " EXP!\n";
    
    int oldLevel = winner.getLevel();
    winner.gainExperience(expGained);
    
    // Check if level up occurred
    if (winner.getLevel() > oldLevel) {
        std::cout << winner.getName() << " grew to level " << winner.getLevel() << "!\n";
    }
}

void Battle::executeTurn(int player1MoveIndex, int player2MoveIndex) {
    Pokemon* p1Pokemon = player1->getActivePokemon();
    Pokemon* p2Pokemon = player2->getActivePokemon();
    
    if (!p1Pokemon || !p2Pokemon || p1Pokemon->isFainted() || p2Pokemon->isFainted()) {
        return;
    }
    
    // Determine turn order
    int firstMover = determineTurnOrder();
    
    // Execute moves in speed order
    if (firstMover == 1) {
        // Player 1 moves first
        if (player1MoveIndex >= 0 && player1MoveIndex < static_cast<int>(p1Pokemon->getMoves().size())) {
            Attack& move = p1Pokemon->getMoves()[player1MoveIndex];
            if (move.canUse() && checkAccuracy(move)) {
                int damage = calculateDamage(*p1Pokemon, *p2Pokemon, move);
                p2Pokemon->takeDamage(damage);
                move.use();
                std::cout << p1Pokemon->getName() << " used " << move.getName() << "!\n";
                std::cout << "Dealt " << damage << " damage to " << p2Pokemon->getName() << "!\n";
            } else if (!move.canUse()) {
                std::cout << "No PP left for " << move.getName() << "!\n";
            } else {
                std::cout << p1Pokemon->getName() << "'s attack missed!\n";
            }
        }
        
        // Check if battle ended and award experience
        if (p2Pokemon->isFainted()) {
            // Player 1's Pokemon defeated opponent - award experience
            awardExperience(*p1Pokemon, *p2Pokemon);
            displayBattleStatus();
            return;
        }
        if (p1Pokemon->isFainted()) {
            // Player 2's Pokemon defeated player 1's Pokemon
            // (In trainer battles, NPCs don't gain EXP, but we could implement it)
            displayBattleStatus();
            return;
        }
        
        // Player 2 moves second
        if (player2MoveIndex >= 0 && player2MoveIndex < static_cast<int>(p2Pokemon->getMoves().size())) {
            Attack& move = p2Pokemon->getMoves()[player2MoveIndex];
            lastEnemyMoveName = move.getName(); // Store enemy move name
            if (move.canUse() && checkAccuracy(move)) {
                int damage = calculateDamage(*p2Pokemon, *p1Pokemon, move);
                p1Pokemon->takeDamage(damage);
                move.use();
                std::cout << p2Pokemon->getName() << " used " << move.getName() << "!\n";
                std::cout << "Dealt " << damage << " damage to " << p1Pokemon->getName() << "!\n";
            } else if (!move.canUse()) {
                std::cout << "No PP left for " << move.getName() << "!\n";
            } else {
                std::cout << p2Pokemon->getName() << "'s attack missed!\n";
            }
        }
    } else {
        // Player 2 moves first
        if (player2MoveIndex >= 0 && player2MoveIndex < static_cast<int>(p2Pokemon->getMoves().size())) {
            Attack& move = p2Pokemon->getMoves()[player2MoveIndex];
            lastEnemyMoveName = move.getName(); // Store enemy move name
            if (move.canUse() && checkAccuracy(move)) {
                int damage = calculateDamage(*p2Pokemon, *p1Pokemon, move);
                p1Pokemon->takeDamage(damage);
                move.use();
                std::cout << p2Pokemon->getName() << " used " << move.getName() << "!\n";
                std::cout << "Dealt " << damage << " damage to " << p1Pokemon->getName() << "!\n";
            } else if (!move.canUse()) {
                std::cout << "No PP left for " << move.getName() << "!\n";
            } else {
                std::cout << p2Pokemon->getName() << "'s attack missed!\n";
            }
        }
        
        // Check if battle ended and award experience
        if (p1Pokemon->isFainted()) {
            // Player 2's Pokemon defeated player 1's Pokemon
            displayBattleStatus();
            return;
        }
        if (p2Pokemon->isFainted()) {
            // Player 1's Pokemon defeated opponent - award experience
            awardExperience(*p1Pokemon, *p2Pokemon);
            displayBattleStatus();
            return;
        }
        
        // Player 1 moves second
        if (player1MoveIndex >= 0 && player1MoveIndex < static_cast<int>(p1Pokemon->getMoves().size())) {
            Attack& move = p1Pokemon->getMoves()[player1MoveIndex];
            if (move.canUse() && checkAccuracy(move)) {
                int damage = calculateDamage(*p1Pokemon, *p2Pokemon, move);
                p2Pokemon->takeDamage(damage);
                move.use();
                std::cout << p1Pokemon->getName() << " used " << move.getName() << "!\n";
                std::cout << "Dealt " << damage << " damage to " << p2Pokemon->getName() << "!\n";
            } else if (!move.canUse()) {
                std::cout << "No PP left for " << move.getName() << "!\n";
            } else {
                std::cout << p1Pokemon->getName() << "'s attack missed!\n";
            }
        }
    }
    
    displayBattleStatus();
}

void Battle::executeEnemyTurn() {
    // Execute only the enemy's turn (for items/switches/failed run)
    Pokemon* p1Pokemon = player1->getActivePokemon();
    Pokemon* p2Pokemon = player2->getActivePokemon();
    
    if (!p1Pokemon || !p2Pokemon || p1Pokemon->isFainted() || p2Pokemon->isFainted()) {
        return;
    }
    
    // Select random move for enemy
    int enemyMoveIndex = 0;
    if (!p2Pokemon->getMoves().empty()) {
        std::uniform_int_distribution<> moveSelect(0, static_cast<int>(p2Pokemon->getMoves().size()) - 1);
        enemyMoveIndex = moveSelect(gen);
    }
    
    // Execute enemy's move
    if (enemyMoveIndex >= 0 && enemyMoveIndex < static_cast<int>(p2Pokemon->getMoves().size())) {
        Attack& move = p2Pokemon->getMoves()[enemyMoveIndex];
        lastEnemyMoveName = move.getName(); // Store enemy move name
        if (move.canUse() && checkAccuracy(move)) {
            int damage = calculateDamage(*p2Pokemon, *p1Pokemon, move);
            p1Pokemon->takeDamage(damage);
            move.use();
            std::cout << p2Pokemon->getName() << " used " << move.getName() << "!\n";
            std::cout << "Dealt " << damage << " damage to " << p1Pokemon->getName() << "!\n";
        } else if (!move.canUse()) {
            std::cout << "No PP left for " << move.getName() << "!\n";
        } else {
            std::cout << p2Pokemon->getName() << "'s attack missed!\n";
        }
    }
    
    displayBattleStatus();
}

void Battle::processFightAction(int moveIndex) {
    Pokemon* active = player1->getActivePokemon();
    if (!active) return;
    
    const auto& moves = active->getMoves();
    if (moveIndex < 0 || moveIndex >= static_cast<int>(moves.size())) {
        std::cout << "Invalid move selection!\n";
        return;
    }
    
    if (!moves[moveIndex].canUse()) {
        std::cout << "No PP left for that move!\n";
        return;
    }
    
    // For NPC, select random move
    Pokemon* enemy = player2->getActivePokemon();
    int enemyMoveIndex = 0;
    if (enemy && !enemy->getMoves().empty()) {
        std::uniform_int_distribution<> moveSelect(0, static_cast<int>(enemy->getMoves().size()) - 1);
        enemyMoveIndex = moveSelect(gen);
    }
    
    executeTurn(moveIndex, enemyMoveIndex);
    state = BattleState::MENU;
}

void Battle::processBagAction(int itemIndex) {
    auto& items = player1->getBag().getItems();
    if (itemIndex < 0 || itemIndex >= static_cast<int>(items.size())) {
        std::cout << "Invalid item selection!\n";
        return;
    }
    
    Item& item = items[itemIndex];
    if (item.getQuantity() <= 0) {
        std::cout << "You don't have any " << item.getName() << "!\n";
        return;
    }
    
    Pokemon* active = player1->getActivePokemon();
    if (!active) return;
    
    // Use item based on type
    switch (item.getType()) {
        case ItemType::POTION:
        case ItemType::SUPER_POTION:
            if (active->isFainted()) {
                std::cout << "Can't use that on a fainted Pokemon!\n";
                return;
            }
            active->heal(item.getEffectValue());
            item.use();
            std::cout << "Used " << item.getName() << "! Restored " 
                      << item.getEffectValue() << " HP!\n";
            break;
        case ItemType::REVIVE:
            if (!active->isFainted()) {
                std::cout << "Can't use that on a Pokemon that hasn't fainted!\n";
                return;
            }
            active->heal(active->getMaxHP() / 2);
            item.use();
            std::cout << "Used " << item.getName() << "! " << active->getName() << " was revived!\n";
            break;
        case ItemType::POKE_BALL:
            // Pokeball usage is handled separately in BattleSequence
            std::cout << "Can't use Poke Ball here!\n";
            return;
        default:
            std::cout << "Item not implemented yet!\n";
            break;
    }
    
    displayBattleStatus();
    // Enemy gets a turn after item use
    executeEnemyTurn();
    state = BattleState::MENU;
}

void Battle::processPokemonAction(int pokemonIndex) {
    if (pokemonIndex < 0 || pokemonIndex >= static_cast<int>(player1->getTeam().size())) {
        std::cout << "Invalid Pokemon selection!\n";
        return;
    }
    
    if (player1->getTeam()[pokemonIndex].isFainted()) {
        std::cout << "That Pokemon has fainted!\n";
        return;
    }
    
    if (pokemonIndex == player1->getActivePokemonIndex()) {
        std::cout << "That Pokemon is already out!\n";
        return;
    }
    
    player1->switchPokemon(pokemonIndex);
    std::cout << "Go! " << player1->getActivePokemon()->getName() << "!\n";
    displayBattleStatus();
    // Enemy gets a turn after switching Pokemon
    executeEnemyTurn();
    state = BattleState::MENU;
}

void Battle::processRunAction() {
    if (attemptRun()) {
        state = BattleState::BATTLE_END;
    } else {
        // Enemy gets a turn after failed run attempt
        executeEnemyTurn();
        state = BattleState::MENU;
    }
}

void Battle::processAction(BattleAction action) {
    switch (action) {
        case BattleAction::FIGHT:
            state = BattleState::FIGHT_MENU;
            displayFightMenu();
            break;
        case BattleAction::BAG:
            state = BattleState::BAG_MENU;
            displayBagMenu();
            break;
        case BattleAction::POKEMON:
            state = BattleState::POKEMON_MENU;
            displayPokemonMenu();
            break;
        case BattleAction::RUN:
            state = BattleState::RUN_CONFIRM;
            processRunAction();
            break;
        default:
            break;
    }
}

void Battle::returnToMainMenu() {
    state = BattleState::MENU;
}

bool Battle::isBattleOver() const {
    return player1->isDefeated() || player2->isDefeated() || state == BattleState::BATTLE_END;
}

Player* Battle::getWinner() const {
    if (player1->isDefeated()) {
        return player2;
    } else if (player2->isDefeated()) {
        return player1;
    }
    return nullptr;
}


#ifndef BATTLE_H
#define BATTLE_H

#include "Player.h"
#include "Type.h"
#include <random>
#include <memory>

enum class BattleAction {
    FIGHT,
    BAG,
    POKEMON,
    RUN,
    NONE
};

enum class BattleState {
    SETUP,
    MENU,
    FIGHT_MENU,
    BAG_MENU,
    POKEMON_MENU,
    RUN_CONFIRM,
    EXECUTING_TURN,
    BATTLE_END
};

class Battle {
private:
    Player* player1;
    Player* player2;
    bool isWildBattle;  // true if player2 is a wild Pokemon (NPC)
    int escapeAttempts;
    BattleState state;
    
    // Random number generation
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> damageRandom;  // 85-100 for damage variance
    std::uniform_int_distribution<> accuracyCheck;  // 1-100 for accuracy
    std::uniform_int_distribution<> runCheck;  // 0-255 for run calculation
    std::uniform_int_distribution<> criticalCheck;  // 1-100 for critical hits
    
    // Battle mechanics
    int calculateDamage(Pokemon& attacker, Pokemon& defender, Attack& move);
    bool checkAccuracy(const Attack& move);
    bool checkCriticalHit();
    bool attemptRun();
    int determineTurnOrder();
    void awardExperience(Pokemon& winner, Pokemon& loser);  // Award EXP when Pokemon is defeated
    
    // Display methods
    void displayBattleStatus() const;
    void displayMainMenu() const;
    void displayFightMenu() const;
    void displayBagMenu() const;
    void displayPokemonMenu() const;
    
public:
    Battle(Player* p1, Player* p2, bool isWild = false);
    
    // Main battle loop
    void startBattle();
    void processAction(BattleAction action);
    void processFightAction(int moveIndex);
    void processBagAction(int itemIndex);
    void processPokemonAction(int pokemonIndex);
    void processRunAction();
    
    // Execute turn
    void executeTurn(int player1MoveIndex, int player2MoveIndex);
    void executeEnemyTurn();  // Execute only enemy's turn (for items/switches/failed run)
    
    // Reset to main menu (for BACK button)
    void returnToMainMenu();
    
    // Getters
    BattleState getState() const { return state; }
    bool isBattleOver() const;
    Player* getWinner() const;
    Player* getPlayer1() const { return player1; }
    Player* getPlayer2() const { return player2; }
    bool getIsWildBattle() const { return isWildBattle; }
};

#endif // BATTLE_H


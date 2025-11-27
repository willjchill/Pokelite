#ifndef BATTLE_SYSTEM_H
#define BATTLE_SYSTEM_H

#include "../game_logic/Battle.h"
#include "../game_logic/Player.h"
#include "../game_logic/Pokemon.h"
#include <QString>
#include <vector>
#include <memory>

// BattleSystem wraps the Battle class and provides UI-friendly interface
class BattleSystem
{
public:
    BattleSystem();
    ~BattleSystem();

    // Initialize battle with players
    void initializeBattle(Player* player1, Player* player2, bool isWild = false);
    
    // Start the battle
    void startBattle();
    
    // Process player actions
    void processAction(BattleAction action);
    void processFightAction(int moveIndex);
    void processBagAction(int itemIndex);
    void processPokemonAction(int pokemonIndex);
    void processRunAction();
    void returnToMainMenu();  // For BACK button
    
    // Getters for UI
    BattleState getState() const;
    bool isBattleOver() const;
    Player* getWinner() const;
    
    // Get current battle info for UI display
    QString getPlayerPokemonName() const;
    QString getEnemyPokemonName() const;
    int getPlayerHP() const;
    int getPlayerMaxHP() const;
    int getEnemyHP() const;
    int getEnemyMaxHP() const;
    
    // Get moves for UI
    std::vector<QString> getPlayerMoves() const;
    std::vector<int> getPlayerMovePP() const;
    std::vector<int> getPlayerMoveMaxPP() const;
    
    // Get items for UI
    std::vector<QString> getBagItems() const;
    std::vector<int> getBagItemQuantities() const;
    
    // Get team for UI
    std::vector<QString> getTeamNames() const;
    std::vector<int> getTeamHP() const;
    std::vector<int> getTeamMaxHP() const;
    std::vector<bool> getTeamFainted() const;
    int getActivePokemonIndex() const;
    
    // Get last battle message (for text display)
    QString getLastMessage() const { return lastMessage; }
    void clearLastMessage() { lastMessage.clear(); }
    
    // Check if waiting for player input
    bool isWaitingForPlayerMove() const;
    bool isWaitingForEnemyTurn() const;
    
    // Update last message based on battle state
    void updateLastMessage();
    
    // Get the underlying Battle object (for advanced use)
    Battle* getBattle() { return battle.get(); }
    const Battle* getBattle() const { return battle.get(); }

private:
    std::unique_ptr<Battle> battle;
    QString lastMessage;
    
    // Helper to convert std::string to QString
    QString toQString(const std::string& str) const;
};

#endif // BATTLE_SYSTEM_H

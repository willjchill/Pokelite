#include "Battle.h"
#include "Player.h"
#include "Pokemon.h"
#include "PokemonData.h"
#include "Attack.h"
#include <iostream>

int main() {
    // Initialize Pokemon and move data from JSON files ONCE at startup
    // This is the only place we explicitly call initializePokemonDataFromJSON()
    // All other functions check the flag and skip if already initialized
    std::cout << "Loading Pokemon data from JSON files (this happens only once)...\n";
    if (!initializePokemonDataFromJSON()) {
        std::cerr << "Error: Failed to load Pokemon data from JSON files!\n";
        std::cerr << "Make sure firered_full_pokedex.json and firered_moves.json are in the current directory.\n";
        return 1;
    }
    std::cout << "Pokemon data loaded successfully! (Data is now cached in memory)\n\n";
    
    // Create player 1
    Player player1("Ash", PlayerType::HUMAN);
    
    // Create Pokemon using species ID (dex number) - moves are automatically learned from JSON data
    // Pikachu is #25, Charizard is #6
    std::cout << "Creating Pikachu (level 10)...\n";
    Pokemon pikachu(25, 10);  // Pikachu at level 10 - will learn moves automatically from JSON
    std::cout << "Pikachu's moves: ";
    for (const auto& move : pikachu.getMoves()) {
        std::cout << move.getName() << " ";
    }
    std::cout << "\n\n";
    player1.addPokemon(pikachu);
    
    std::cout << "Creating Charizard (level 12)...\n";
    Pokemon charizard(6, 12);  // Charizard at level 12 - will learn moves automatically from JSON
    std::cout << "Charizard's moves: ";
    for (const auto& move : charizard.getMoves()) {
        std::cout << move.getName() << " ";
    }
    std::cout << "\n\n";
    player1.addPokemon(charizard);
    
    // Create opponent (NPC)
    Player opponent("Gary", PlayerType::NPC);
    
    // Create opponent's Pokemon using species IDs from JSON
    // Squirtle is #7, Bulbasaur is #1
    std::cout << "Creating Squirtle (level 10)...\n";
    Pokemon squirtle(7, 10);  // Squirtle at level 10
    std::cout << "Squirtle's moves: ";
    for (const auto& move : squirtle.getMoves()) {
        std::cout << move.getName() << " ";
    }
    std::cout << "\n\n";
    opponent.addPokemon(squirtle);
    
    std::cout << "Creating Bulbasaur (level 11)...\n";
    Pokemon bulbasaur(1, 11);  // Bulbasaur at level 11
    std::cout << "Bulbasaur's moves: ";
    for (const auto& move : bulbasaur.getMoves()) {
        std::cout << move.getName() << " ";
    }
    std::cout << "\n\n";
    opponent.addPokemon(bulbasaur);
    
    // Create battle (trainer battle, not wild)
    Battle battle(&player1, &opponent, false);
    
    // Start battle
    battle.startBattle();
    
    // Simple battle loop (in a real game, this would be handled by UI)
    std::cout << "\n=== BATTLE DEMO ===\n";
    std::cout << "This is a simplified demo. In a full implementation,\n";
    std::cout << "you would integrate this with a proper menu system.\n\n";
    
    // Example: Player uses first move
    std::cout << "Player chooses to FIGHT and selects move 1...\n";
    battle.processFightAction(0);
    
    std::cout << "\n--- Turn Complete ---\n";
    
    // Check if battle is over
    if (battle.isBattleOver()) {
        Player* winner = battle.getWinner();
        if (winner) {
            std::cout << "\n" << winner->getName() << " wins the battle!\n";
        } else {
            std::cout << "\nBattle ended (player ran away).\n";
        }
    }
    
    return 0;
}


#include "Player.h"

Player::Player(int x, int y)
    : Entity(x, y, 32, 32)
{
    // Default pokemon
    party.push_back(Pokemon("Pikachu", 5));
}

void Player::addPokemon(const Pokemon& p) {
    party.push_back(p);
}

void Player::moveUp()    { y -= speed; }
void Player::moveDown()  { y += speed; }
void Player::moveLeft()  { x -= speed; }
void Player::moveRight() { x += speed; }

#include "battle_sequence.h"
#include "Battle_logic/PokemonData.h"
#include "Battle_logic/Item.h"
#include <QDebug>
#include <QBrush>
#include <QPen>
#include <QTimer>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QFont>
#include <QApplication>
#include <cmath>


BattleSequence::BattleSequence(QGraphicsScene *scene, QGraphicsView *view)
    : QObject(nullptr), battleScene(scene), view(view), animations(this)
{
    connect(&battleTextTimer, &QTimer::timeout, this, [this]() {
        if (!battleTextItem) {
            battleTextTimer.stop();
            return;
        }
        if (battleTextIndex >= fullBattleText.size()) {
            battleTextTimer.stop();
            return;
        }
        battleTextIndex++;
        battleTextItem->setPlainText(fullBattleText.left(battleTextIndex));
    });
}


BattleSequence::~BattleSequence()
{
    closeBattle();
}

QString BattleSequence::capitalizeFirst(const QString& str) const
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1).toLower();
}

void BattleSequence::startBattle(Player* player, Player* enemy, BattleSystem* bs)
{
    gamePlayer = player;
    enemyPlayer = enemy;
    battleSystem = bs;

    // Reset state
    inBattle = true;
    inBattleMenu = true;
    inBagMenu = false;
    inPokemonMenu = false;
    battleMenuIndex = 0;
    fullBattleText.clear();
    battleTextIndex = 0;

    // Clean any leftover menus
    destroyMoveMenu();
    destroyBagMenu();
    destroyPokemonMenu();

    // New battle scene
    battleScene = new QGraphicsScene(0, 0, 480, 272, nullptr);

    // Background
    QPixmap battleBg(":/assets/battle/battle_bg.png");
    if (battleBg.isNull()) {
        battleBg = QPixmap(480, 272);
        battleBg.fill(Qt::black);
    }
    battleScene->addPixmap(battleBg)->setZValue(0);

    // Player trainer sprite
    QPixmap trainerPx(":/assets/battle/trainer.png");
    if (!trainerPx.isNull()) {
        float sx = 120.0f / trainerPx.width();
        float sy = 120.0f / trainerPx.height();
        float scale = std::min(sx, sy);

        battleTrainerItem = battleScene->addPixmap(trainerPx);
        battleTrainerItem->setScale(scale);

        qreal h = trainerPx.height() * scale;
        battleTrainerItem->setPos(20, 272 - h - 60);
        battleTrainerItem->setZValue(0);
    }

    // Player's Pokemon sprite
    const Pokemon* playerPokemon = gamePlayer->getActivePokemon();
    if (playerPokemon) {
        QString playerSpritePath = QString::fromStdString(playerPokemon->getBackSpritePath());
        QPixmap playerPx(playerSpritePath);
        if (playerPx.isNull()) {
            QString spriteDir = QString::fromStdString(playerPokemon->getSpriteDir());
            playerSpritePath = ":/" + spriteDir + "/back.png";
            playerPx = QPixmap(playerSpritePath);
        }
        if (playerPx.isNull()) {
            playerPx = QPixmap(":/Battle/assets/pokemon_sprites/006_charizard/back.png");
        }
        if (!playerPx.isNull()) {
            float sx = 120.0f / playerPx.width();
            float sy = 120.0f / playerPx.height();
            float scale = std::min(sx, sy);

            battlePlayerPokemonItem = battleScene->addPixmap(playerPx);
            battlePlayerPokemonItem->setScale(scale);

            qreal h = playerPx.height() * scale;
            battlePlayerPokemonItem->setPos(100, 272 - h - 50);
            battlePlayerPokemonItem->setZValue(2);
        }
    }

    // Enemy Pokémon sprite
    const Pokemon* enemyPokemon = enemyPlayer->getActivePokemon();
    QString enemySpritePath = ":/Battle/assets/pokemon_sprites/006_charizard/front.png";
    if (enemyPokemon) {
        enemySpritePath = QString::fromStdString(enemyPokemon->getFrontSpritePath());
    }
    QPixmap enemyPx(enemySpritePath);
    if (enemyPx.isNull()) {
        if (enemyPokemon) {
            QString spriteDir = QString::fromStdString(enemyPokemon->getSpriteDir());
            enemySpritePath = ":/" + spriteDir + "/front.png";
            enemyPx = QPixmap(enemySpritePath);
        }
        if (enemyPx.isNull()) {
            enemyPx = QPixmap(":/Battle/assets/pokemon_sprites/006_charizard/front.png");
        }
    }
    if (!enemyPx.isNull()) {
        float sx = 140.0f / enemyPx.width();
        float sy = 140.0f / enemyPx.height();
        float scale = std::min(sx, sy);

        battleEnemyItem = battleScene->addPixmap(enemyPx);
        battleEnemyItem->setScale(scale);

        qreal w = enemyPx.width() * scale;
        battleEnemyItem->setPos(480 - w - 30, 30);
        battleEnemyItem->setZValue(2);
    }

    // Setup UI
    setupBattleUI();

    // Switch to battle scene
    view->setScene(battleScene);
    view->setFixedSize(480, 272);

    // Start the battle
    battleSystem->startBattle();
    
    // Update UI with initial state
    updateBattleUI();

    // Animations
    animations.battleZoomReveal(this);
    animations.animateBattleEntrances(this);
    animations.slideInCommandMenu(this);
}

void BattleSequence::closeBattle()
{
    // Note: battleSystem and enemyPlayer are owned by MainWindow, not BattleSequence
    // So we don't delete them here
    
    inBattle = false;
    inBattleMenu = false;
    inBagMenu = false;
    inPokemonMenu = false;
    
    destroyMoveMenu();
    destroyBagMenu();
    destroyPokemonMenu();
    
    emit battleEnded();
}

void BattleSequence::setupBattleUI()
{
    QFont font("Pokemon Fire Red", 10, QFont::Bold);

    // Enemy HP box
    QPixmap enemyBox(":/assets/battle/ui/hp_box_enemy.png");
    enemyHpBackSprite = battleScene->addPixmap(enemyBox);
    enemyHpBackSprite->setPos(20, 10);
    enemyHpBackSprite->setZValue(2);

    enemyHpMask = new QGraphicsRectItem(0, 0, 64, 8);
    enemyHpMask->setBrush(QColor("#F8F8F0"));
    enemyHpMask->setPen(Qt::NoPen);
    enemyHpMask->setPos(20 + 30, 10 + 18);
    enemyHpMask->setZValue(3);
    battleScene->addItem(enemyHpMask);

    enemyHpFill = new QGraphicsRectItem(0, 0, 62, 6);
    enemyHpFill->setBrush(QColor("#7BD77B"));
    enemyHpFill->setPen(Qt::NoPen);
    enemyHpFill->setPos(20 + 31, 10 + 19);
    enemyHpFill->setZValue(4);
    battleScene->addItem(enemyHpFill);

    // Player HP box
    QPixmap playerBox(":/assets/battle/ui/hp_box_player.png");
    playerHpBackSprite = battleScene->addPixmap(playerBox);
    playerHpBackSprite->setPos(260, 140);
    playerHpBackSprite->setZValue(2);

    playerHpMask = new QGraphicsRectItem(0, 0, 88, 8);
    playerHpMask->setBrush(QColor("#F8F8F0"));
    playerHpMask->setPen(Qt::NoPen);
    playerHpMask->setPos(260 + 42, 140 + 29);
    playerHpMask->setZValue(3);
    battleScene->addItem(playerHpMask);

    playerHpFill = new QGraphicsRectItem(0, 0, 86, 6);
    playerHpFill->setBrush(QColor("#7BD77B"));
    playerHpFill->setPen(Qt::NoPen);
    playerHpFill->setPos(260 + 43, 140 + 28);
    playerHpFill->setZValue(4);
    battleScene->addItem(playerHpFill);

    // Enemy Pokemon name text
    enemyPokemonNameText = new QGraphicsTextItem();
    enemyPokemonNameText->setDefaultTextColor(Qt::black);
    enemyPokemonNameText->setFont(font);
    enemyPokemonNameText->setPos(20 + 5, 10 + 2);
    enemyPokemonNameText->setZValue(5);
    battleScene->addItem(enemyPokemonNameText);

    // Player Pokemon name text
    playerPokemonNameText = new QGraphicsTextItem();
    playerPokemonNameText->setDefaultTextColor(Qt::black);
    playerPokemonNameText->setFont(font);
    playerPokemonNameText->setPos(260 + 5, 140 + 2);
    playerPokemonNameText->setZValue(5);
    battleScene->addItem(playerPokemonNameText);

    // Dialogue box
    QPixmap dialogPx(":/assets/battle/ui/dialogue_box.png");
    dialogueBoxSprite = battleScene->addPixmap(dialogPx);
    {
        double sx = 480.0 / dialogPx.width();
        double sy = 72.0 / dialogPx.height();
        dialogueBoxSprite->setScale(std::min(sx, sy));

        dialogueBoxSprite->setPos(
            0,
            272 - dialogueBoxSprite->boundingRect().height() * dialogueBoxSprite->scale()
        );
        dialogueBoxSprite->setZValue(2);
    }

    battleTextItem = new QGraphicsTextItem();
    battleTextItem->setDefaultTextColor(Qt::white);
    battleTextItem->setFont(font);
    battleTextItem->setPos(dialogueBoxSprite->pos().x() + 18,
                           dialogueBoxSprite->pos().y() + 14);
    battleTextItem->setZValue(3);
    battleScene->addItem(battleTextItem);

    fullBattleText = "What will " + (battleSystem ? battleSystem->getPlayerPokemonName() : "BULBASAUR") + " do?";
    battleTextIndex = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(30);

    // Command box
    QPixmap cmdBox(":/assets/battle/ui/command_box.png");
    commandBoxSprite = battleScene->addPixmap(cmdBox);
    commandBoxSprite->setPos(480 - 160 - 50, 272 - 64 - 17);
    commandBoxSprite->setScale(1.3);
    commandBoxSprite->setZValue(2);

    // Invisible anchors for the 4 command slots
    int px[4] = {25, 100, 25, 100};
    int py[4] = {18, 18, 42, 42};

    battleMenuOptions.clear();
    for (int i = 0; i < 4; ++i) {
        QGraphicsTextItem *t = new QGraphicsTextItem(QString::number(i));
        t->setVisible(false);
        t->setFont(font);
        t->setPos(commandBoxSprite->pos().x() + px[i] * commandBoxSprite->scale(),
                  commandBoxSprite->pos().y() + py[i] * commandBoxSprite->scale());
        t->setZValue(3);
        battleScene->addItem(t);
        battleMenuOptions.push_back(t);
    }

    // Cursor arrow
    QPixmap arrow(":/assets/battle/ui/arrow_cursor.png");
    battleCursorSprite = battleScene->addPixmap(arrow);
    battleCursorSprite->setScale(2.0);
    battleCursorSprite->setZValue(5);
    battleMenuIndex = 0;

    updateBattleCursor();
    updateBattleUI();
}

void BattleSequence::updateBattleUI()
{
    if (!battleSystem) return;
    
    // Update Pokemon names
    if (enemyPokemonNameText) {
        QString enemyName = battleSystem->getEnemyPokemonName();
        if (enemyName.isEmpty()) {
            enemyName = "FOE";
        } else {
            enemyName = capitalizeFirst(enemyName);
        }
        enemyPokemonNameText->setPlainText(enemyName);
    }
    
    if (playerPokemonNameText) {
        QString playerName = battleSystem->getPlayerPokemonName();
        if (playerName.isEmpty()) {
            playerName = "POKEMON";
        } else {
            playerName = capitalizeFirst(playerName);
        }
        playerPokemonNameText->setPlainText(playerName);
    }
    
    // Update HP bars
    int playerHP = battleSystem->getPlayerHP();
    int playerMaxHP = battleSystem->getPlayerMaxHP();
    int enemyHP = battleSystem->getEnemyHP();
    int enemyMaxHP = battleSystem->getEnemyMaxHP();
    
    if (playerMaxHP > 0 && playerHpFill) {
        float playerHpPercent = (float)playerHP / (float)playerMaxHP;
        playerHpFill->setRect(0, 0, 86 * playerHpPercent, 6);
        setHpColor(playerHpFill, playerHpPercent);
    }
    
    if (enemyMaxHP > 0 && enemyHpFill) {
        float enemyHpPercent = (float)enemyHP / (float)enemyMaxHP;
        enemyHpFill->setRect(0, 0, 62 * enemyHpPercent, 6);
        setHpColor(enemyHpFill, enemyHpPercent);
    }
}

void BattleSequence::setHpColor(QGraphicsRectItem *hpBar, float hpPercent)
{
    if (hpPercent > 0.5f)       hpBar->setBrush(QColor("#3CD75F"));
    else if (hpPercent > 0.2f)  hpBar->setBrush(QColor("#FFCB43"));
    else                        hpBar->setBrush(QColor("#FF4949"));
}

void BattleSequence::setBattleText(const QString &text)
{
    fullBattleText = text;
    battleTextIndex = 0;
    if (battleTextItem) {
        battleTextItem->setPlainText("");
    }
}

void BattleSequence::startTextAnimation()
{
    battleTextTimer.start(22);
}

void BattleSequence::handleBattleKey(QKeyEvent *event)
{
    if (!inBattle || !battleSystem)
        return;

    if (battleSystem->isWaitingForEnemyTurn())
        return;

    bool inMoveMenu = battleSystem->isWaitingForPlayerMove();
    
    int maxIndex = 0;
    if (inBagMenu) {
        maxIndex = bagMenuOptions.size();
    } else if (inPokemonMenu) {
        maxIndex = pokemonMenuOptions.size();
    } else if (inMoveMenu) {
        maxIndex = moveMenuOptions.size();
    } else if (inBattleMenu) {
        maxIndex = battleMenuOptions.size();
    }
    
    if (maxIndex == 0)
        return;

    int key = event->key();

    if (key == Qt::Key_Left || key == Qt::Key_A) {
        if (battleMenuIndex % 2 == 1) {
            battleMenuIndex--;
        }
    }
    else if (key == Qt::Key_Right || key == Qt::Key_D) {
        if (battleMenuIndex % 2 == 0 && battleMenuIndex + 1 < maxIndex) {
            battleMenuIndex++;
        }
    }
    else if (key == Qt::Key_Up || key == Qt::Key_W) {
        if (battleMenuIndex >= 2) {
            battleMenuIndex -= 2;
        }
    }
    else if (key == Qt::Key_Down || key == Qt::Key_S) {
        if (battleMenuIndex + 2 < maxIndex) {
            battleMenuIndex += 2;
        }
    }
    else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (inBagMenu) {
            playerSelectedBagItem(battleMenuIndex);
        } else if (inPokemonMenu) {
            playerSelectedPokemon(battleMenuIndex);
        } else if (inMoveMenu) {
            playerChoseMove(battleMenuIndex);
        } else if (inBattleMenu) {
            playerSelectedOption(battleMenuIndex);
        }
        return;
    }

    updateBattleCursor();
}

void BattleSequence::updateBattleCursor()
{
    if (!battleCursorSprite) return;

    QGraphicsTextItem *target = nullptr;
    
    if (inBagMenu && !bagMenuOptions.isEmpty() && battleMenuIndex < bagMenuOptions.size()) {
        target = bagMenuOptions[battleMenuIndex];
    } else if (inPokemonMenu && !pokemonMenuOptions.isEmpty() && battleMenuIndex < pokemonMenuOptions.size()) {
        target = pokemonMenuOptions[battleMenuIndex];
    } else if (battleSystem && battleSystem->isWaitingForPlayerMove() && !moveMenuOptions.isEmpty() && battleMenuIndex < moveMenuOptions.size()) {
        target = moveMenuOptions[battleMenuIndex];
    } else if (!battleMenuOptions.isEmpty() && battleMenuIndex < battleMenuOptions.size()) {
        target = battleMenuOptions[battleMenuIndex];
    }
    
    if (target) {
        battleCursorSprite->setPos(target->pos().x() - 28,
                                   target->pos().y() - 2);
    }
}

void BattleSequence::playerSelectedOption(int index)
{
    if (!battleSystem) return;
    
    if (index == 0) { // FIGHT
        battleSystem->processAction(BattleAction::FIGHT);
        
        destroyBagMenu();
        destroyPokemonMenu();
        destroyMoveMenu();
        
        std::vector<QString> moves = battleSystem->getPlayerMoves();
        std::vector<int> pp = battleSystem->getPlayerMovePP();
        std::vector<int> maxPP = battleSystem->getPlayerMoveMaxPP();
        
        QFont f("Pokemon Fire Red", 9, QFont::Bold);
        
        const qreal boxW = 240, boxH = 80;
        qreal boxX = dialogueBoxSprite->pos().x() + 10;
        qreal boxY = dialogueBoxSprite->pos().y() - boxH - 4;
        
        moveMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
        moveMenuRect->setBrush(Qt::white);
        moveMenuRect->setPen(QPen(Qt::black, 2));
        moveMenuRect->setZValue(3);
        battleScene->addItem(moveMenuRect);
        
        moveMenuOptions.clear();
        int numMoves = std::min(4, (int)moves.size() + 1);
        for (int i = 0; i < numMoves; ++i) {
            QString label;
            if (i < (int)moves.size()) {
                QString moveName = moves[i];
                if (moveName.length() > 12) {
                    moveName = moveName.left(10) + "...";
                }
                label = moveName + "\nPP " + QString::number(pp[i]) + "/" + QString::number(maxPP[i]);
            } else {
                label = "BACK";
            }
            
            QGraphicsTextItem *t = new QGraphicsTextItem(label);
            t->setFont(f);
            t->setDefaultTextColor(Qt::black);
            
            int row = i / 2;
            int col = i % 2;
            t->setPos(boxX + 12 + col * 115,
                      boxY + 10 + row * 35);
            t->setZValue(4);
            
            battleScene->addItem(t);
            moveMenuOptions.push_back(t);
        }
        
        inBattleMenu = false;
        battleMenuIndex = 0;
        
        setBattleText(battleSystem->getLastMessage().isEmpty() ? "Choose a move!" : battleSystem->getLastMessage());
        startTextAnimation();
        
        updateBattleCursor();
        return;
    }
    
    if (index == 1) { // BAG
        battleSystem->processAction(BattleAction::BAG);
        showBagMenu();
        return;
    }
    
    if (index == 2) { // POKEMON
        battleSystem->processAction(BattleAction::POKEMON);
        showPokemonMenu();
        return;
    }
    
    if (index == 3) { // RUN
        battleSystem->processRunAction();
        
        bool runSuccessful = battleSystem->isBattleOver();
        
        setBattleText(battleSystem->getLastMessage().isEmpty() ? (runSuccessful ? "Got away safely!" : "Can't escape!") : battleSystem->getLastMessage());
        startTextAnimation();
        
        inBattleMenu = false;
        
        if (runSuccessful) {
            QTimer::singleShot(1000, [=]() {
                fadeOutBattleScreen([=]() {
                    closeBattle();
                });
            });
        } else {
            QTimer::singleShot(1000, [=]() {
                enemyTurn();
            });
        }
        return;
    }
}

void BattleSequence::playerChoseMove(int moveIndex)
{
    if (!battleSystem) return;
    
    std::vector<QString> moves = battleSystem->getPlayerMoves();
    
    if (moveIndex >= (int)moves.size()) {
        destroyMoveMenu();
        destroyBagMenu();
        destroyPokemonMenu();
        
        battleSystem->returnToMainMenu();
        
        inBattleMenu = true;
        inBagMenu = false;
        inPokemonMenu = false;
        battleMenuIndex = 0;
        
        setBattleText("What will " + battleSystem->getPlayerPokemonName() + " do?");
        startTextAnimation();
        
        updateBattleCursor();
        return;
    }
    
    QString moveName = moves[moveIndex];
    setBattleText(battleSystem->getPlayerPokemonName() + " used " + moveName + "!");
    startTextAnimation();
    
    destroyMoveMenu();
    inBattleMenu = false;
    
    battleSystem->processFightAction(moveIndex);
    
    updateBattleUI();
    
    if (battleSystem->isBattleOver()) {
        Player* winner = battleSystem->getWinner();
        if (winner == gamePlayer) {
            // Player won - remove fainted Pokemon from team
            if (gamePlayer) {
                auto& team = gamePlayer->getTeam();
                // Remove fainted Pokemon (iterate backwards to avoid index issues)
                for (int i = static_cast<int>(team.size()) - 1; i >= 0; --i) {
                    if (team[i].isFainted()) {
                        gamePlayer->removePokemon(i);
                    }
                }
            }
            setBattleText("The foe fainted!");
        } else {
            // Player lost - check if all Pokemon are fainted
            if (gamePlayer && gamePlayer->isDefeated()) {
                setBattleText("You have no Pokemon left!\nYou failed the game!");
                startTextAnimation();
                
                QTimer::singleShot(2000, [=]() {
                    fadeOutBattleScreen([=]() {
                        closeBattle();
                        QApplication::exit(0);
                    });
                });
                return;
            } else {
                setBattleText(battleSystem->getPlayerPokemonName() + " fainted...");
            }
        }
        startTextAnimation();
        
        QTimer::singleShot(1200, [=]() {
            fadeOutBattleScreen([=]() {
                closeBattle();
            });
        });
        return;
    }
    
    // Show enemy's move message first (enemy already moved during processFightAction)
    // Then check if player's Pokemon fainted and auto-switch
    QString enemyName = battleSystem->getEnemyPokemonName();
    if (enemyName.isEmpty()) {
        enemyName = "The foe";
    }
    
    QString enemyMoveName = battleSystem->getEnemyLastMoveName();
    if (enemyMoveName.isEmpty()) {
        enemyMoveName = "a move";
    } else {
        enemyMoveName = capitalizeFirst(enemyMoveName);
    }
    
    setBattleText(enemyName + " used " + enemyMoveName + "!");
    startTextAnimation();
    
    QTimer::singleShot(1000, [=]() {
        updateBattleUI();
        
        // Check if player's Pokemon fainted and auto-switch
        if (gamePlayer && gamePlayer->getActivePokemon() && gamePlayer->getActivePokemon()->isFainted()) {
            // Show fainted message first
            QString faintedName = battleSystem->getPlayerPokemonName();
            setBattleText(faintedName + " fainted!");
            startTextAnimation();
            
            QTimer::singleShot(1000, [=]() {
                bool switched = checkAndAutoSwitchPokemon();
                if (!switched) {
                    // No Pokemon available - game over
                    if (gamePlayer->isDefeated()) {
                        setBattleText("You have no Pokemon left!\nYou failed the game!");
                        startTextAnimation();
                        
                        QTimer::singleShot(2000, [=]() {
                            fadeOutBattleScreen([=]() {
                                closeBattle();
                                QApplication::exit(0);
                            });
                        });
                        return;
                    }
                } else {
                    // Successfully switched - show message and return to menu
                    setBattleText("Go! " + battleSystem->getPlayerPokemonName() + "!");
                    startTextAnimation();
                    
                    QTimer::singleShot(1000, [=]() {
                        setBattleText("What will " + battleSystem->getPlayerPokemonName() + " do?");
                        startTextAnimation();
                        
                        inBattleMenu = true;
                        battleMenuIndex = 0;
                        
                        destroyMoveMenu();
                        updateBattleCursor();
                    });
                    return;
                }
            });
            return;
        }
        
        // Player's Pokemon is still alive, return to menu
        setBattleText("What will " + battleSystem->getPlayerPokemonName() + " do?");
        startTextAnimation();
        
        inBattleMenu = true;
        battleMenuIndex = 0;
        
        destroyMoveMenu();
        updateBattleCursor();
    });
}

void BattleSequence::enemyTurn()
{
    if (!battleSystem) return;
    
    QString enemyName = battleSystem->getEnemyPokemonName();
    if (enemyName.isEmpty()) {
        enemyName = "The foe";
    }
    
    // Get the actual move name
    QString moveName = battleSystem->getEnemyLastMoveName();
    if (moveName.isEmpty()) {
        moveName = "a move";
    } else {
        moveName = capitalizeFirst(moveName);
    }
    
    setBattleText(enemyName + " used " + moveName + "!");
    startTextAnimation();
    
    updateBattleUI();
    
    if (battleSystem->isBattleOver()) {
        Player* winner = battleSystem->getWinner();
        if (winner == gamePlayer) {
            // Player won - remove fainted Pokemon from team
            if (gamePlayer) {
                auto& team = gamePlayer->getTeam();
                // Remove fainted Pokemon (iterate backwards to avoid index issues)
                for (int i = static_cast<int>(team.size()) - 1; i >= 0; --i) {
                    if (team[i].isFainted()) {
                        gamePlayer->removePokemon(i);
                    }
                }
            }
            setBattleText("The foe fainted!");
        } else {
            // Player lost - check if all Pokemon are fainted
            if (gamePlayer && gamePlayer->isDefeated()) {
                setBattleText("You have no Pokemon left!\nYou failed the game!");
                startTextAnimation();
                
                inBattleMenu = false;
                
                QTimer::singleShot(2000, [=]() {
                    fadeOutBattleScreen([=]() {
                        closeBattle();
                        QApplication::exit(0);
                    });
                });
                return;
            } else {
                setBattleText(battleSystem->getPlayerPokemonName() + " fainted...");
            }
        }
        startTextAnimation();
        
        inBattleMenu = false;
        
        QTimer::singleShot(1400, [=]() {
            fadeOutBattleScreen([=]() {
                closeBattle();
            });
        });
        return;
    }
    
    // Check if player's Pokemon fainted and auto-switch
    if (gamePlayer && gamePlayer->getActivePokemon() && gamePlayer->getActivePokemon()->isFainted()) {
        // Show fainted message first
        QString faintedName = battleSystem->getPlayerPokemonName();
        setBattleText(faintedName + " fainted!");
        startTextAnimation();
        
        QTimer::singleShot(1000, [=]() {
            bool switched = checkAndAutoSwitchPokemon();
            if (!switched) {
                // No Pokemon available - game over
                if (gamePlayer->isDefeated()) {
                    setBattleText("You have no Pokemon left!\nYou failed the game!");
                    startTextAnimation();
                    
                    inBattleMenu = false;
                    
                    QTimer::singleShot(2000, [=]() {
                        fadeOutBattleScreen([=]() {
                            closeBattle();
                            QApplication::exit(0);
                        });
                    });
                    return;
                }
            } else {
                // Successfully switched - show message
                setBattleText("Go! " + battleSystem->getPlayerPokemonName() + "!");
                startTextAnimation();
                
                QTimer::singleShot(1000, [=]() {
                    setBattleText("What will " + battleSystem->getPlayerPokemonName() + " do?");
                    startTextAnimation();
                    
                    inBattleMenu = true;
                    battleMenuIndex = 0;
                    
                    destroyMoveMenu();
                    updateBattleCursor();
                });
                return;
            }
        });
        return;
    }
    
    QTimer::singleShot(900, [=]() {
        setBattleText("What will " + battleSystem->getPlayerPokemonName() + " do?");
        startTextAnimation();
        
        inBattleMenu = true;
        battleMenuIndex = 0;
        
        destroyMoveMenu();
        updateBattleCursor();
    });
}

void BattleSequence::showBagMenu()
{
    if (!battleSystem || !battleScene) return;
    
    destroyMoveMenu();
    destroyPokemonMenu();
    
    std::vector<QString> items = battleSystem->getBagItems();
    std::vector<int> quantities = battleSystem->getBagItemQuantities();
    
    QFont f("Pokemon Fire Red", 9, QFont::Bold);
    
    const qreal boxW = 240, boxH = 120;
    qreal boxX = dialogueBoxSprite->pos().x() + 10;
    qreal boxY = dialogueBoxSprite->pos().y() - boxH - 4;
    
    bagMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    bagMenuRect->setBrush(Qt::white);
    bagMenuRect->setPen(QPen(Qt::black, 2));
    bagMenuRect->setZValue(3);
    battleScene->addItem(bagMenuRect);
    
    bagMenuOptions.clear();
    bagMenuItemIndices.clear();
    
    // Get actual bag items to map indices correctly
    if (battleSystem && battleSystem->getBattle() && battleSystem->getBattle()->getPlayer1()) {
        const auto& allItems = battleSystem->getBattle()->getPlayer1()->getBag().getItems();
        int menuIndex = 0;
        for (size_t i = 0; i < allItems.size() && menuIndex < 6; ++i) {
            if (allItems[i].getQuantity() > 0) {
                QString itemName = QString::fromStdString(allItems[i].getName());
                if (itemName.length() > 15) {
                    itemName = itemName.left(13) + "...";
                }
                QString label = itemName + " x" + QString::number(allItems[i].getQuantity());
                
                QGraphicsTextItem *t = new QGraphicsTextItem(label);
                t->setFont(f);
                t->setDefaultTextColor(Qt::black);
                
                int row = menuIndex / 2;
                int col = menuIndex % 2;
                t->setPos(boxX + 12 + col * 115,
                          boxY + 10 + row * 20);
                t->setZValue(4);
                
                battleScene->addItem(t);
                bagMenuOptions.push_back(t);
                bagMenuItemIndices.push_back(static_cast<int>(i)); // Store original index
                menuIndex++;
            }
        }
        
        // Add BACK option
        if (menuIndex < 6) {
            QGraphicsTextItem *t = new QGraphicsTextItem("BACK");
            t->setFont(f);
            t->setDefaultTextColor(Qt::black);
            int row = menuIndex / 2;
            int col = menuIndex % 2;
            t->setPos(boxX + 12 + col * 115,
                      boxY + 10 + row * 20);
            t->setZValue(4);
            battleScene->addItem(t);
            bagMenuOptions.push_back(t);
            bagMenuItemIndices.push_back(-1); // -1 means BACK
        }
    }
    
    inBagMenu = true;
    inBattleMenu = false;
    inPokemonMenu = false;
    battleMenuIndex = 0;
    
    setBattleText("Choose an item!");
    startTextAnimation();
    
    updateBattleCursor();
}

void BattleSequence::showPokemonMenu()
{
    if (!battleSystem || !battleScene) return;
    
    destroyMoveMenu();
    destroyBagMenu();
    
    std::vector<QString> names = battleSystem->getTeamNames();
    std::vector<int> hp = battleSystem->getTeamHP();
    std::vector<int> maxHP = battleSystem->getTeamMaxHP();
    std::vector<int> levels = battleSystem->getTeamLevels();
    std::vector<bool> fainted = battleSystem->getTeamFainted();
    int activeIndex = battleSystem->getActivePokemonIndex();
    
    QFont f("Pokemon Fire Red", 9, QFont::Bold);
    
    const qreal boxW = 240, boxH = 140;
    qreal boxX = dialogueBoxSprite->pos().x() + 10;
    qreal boxY = dialogueBoxSprite->pos().y() - boxH - 4;
    
    pokemonMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    pokemonMenuRect->setBrush(Qt::white);
    pokemonMenuRect->setPen(QPen(Qt::black, 2));
    pokemonMenuRect->setZValue(3);
    battleScene->addItem(pokemonMenuRect);
    
    pokemonMenuOptions.clear();
    int numPokemon = std::min(6, (int)names.size() + 1);
    for (int i = 0; i < numPokemon; ++i) {
        QString label;
        if (i < (int)names.size()) {
            QString pokemonName = capitalizeFirst(names[i]);
            if (pokemonName.length() > 10) {
                pokemonName = pokemonName.left(8) + "...";
            }
            QString status = fainted[i] ? "FAINTED" : (i == activeIndex ? "ACTIVE" : "");
            label = pokemonName + " Lv" + QString::number(levels[i]) + "\nHP " + QString::number(hp[i]) + "/" + QString::number(maxHP[i]);
            if (!status.isEmpty()) {
                label += "\n" + status;
            }
        } else {
            label = "BACK";
        }
        
        QGraphicsTextItem *t = new QGraphicsTextItem(label);
        t->setFont(f);
        // Make fainted Pokemon appear grayed out
        if (i < (int)names.size() && fainted[i]) {
            t->setDefaultTextColor(Qt::gray);
        } else {
            t->setDefaultTextColor(Qt::black);
        }
        
        int row = i / 2;
        int col = i % 2;
        t->setPos(boxX + 12 + col * 115,
                  boxY + 10 + row * 25);
        t->setZValue(4);
        
        battleScene->addItem(t);
        pokemonMenuOptions.push_back(t);
    }
    
    inPokemonMenu = true;
    inBattleMenu = false;
    inBagMenu = false;
    battleMenuIndex = 0;
    
    setBattleText("Choose a Pokemon!");
    startTextAnimation();
    
    updateBattleCursor();
}

void BattleSequence::playerSelectedBagItem(int index)
{
    if (!battleSystem) return;
    
    // Check if BACK was selected
    if (index >= bagMenuItemIndices.size() || bagMenuItemIndices[index] == -1) {
        destroyBagMenu();
        
        battleSystem->returnToMainMenu();
        
        inBagMenu = false;
        inBattleMenu = true;
        battleMenuIndex = 0;
        
        setBattleText("What will " + battleSystem->getPlayerPokemonName() + " do?");
        startTextAnimation();
        
        updateBattleCursor();
        return;
    }
    
    // Get the actual bag item index from the mapping
    int actualItemIndex = bagMenuItemIndices[index];
    
    // Check if it's a Pokeball
    if (battleSystem->getBattle() && battleSystem->getBattle()->getPlayer1()) {
        const auto& items = battleSystem->getBattle()->getPlayer1()->getBag().getItems();
        if (actualItemIndex >= 0 && actualItemIndex < static_cast<int>(items.size())) {
            const auto& item = items[actualItemIndex];
            if (item.getType() == ItemType::POKE_BALL) {
                // Handle catching Pokemon
                attemptCatchPokemon(actualItemIndex);
                return;
            }
        }
    }
    
    // Use regular item (potion, etc.)
    battleSystem->processBagAction(actualItemIndex);
    destroyBagMenu();
    
    inBagMenu = false;
    inBattleMenu = false;
    battleMenuIndex = 0;
    
    setBattleText(battleSystem->getLastMessage().isEmpty() ? "Used item!" : battleSystem->getLastMessage());
    startTextAnimation();
    
    updateBattleUI();
    
    QTimer::singleShot(1000, [=]() {
        enemyTurn();
    });
}

void BattleSequence::playerSelectedPokemon(int index)
{
    if (!battleSystem) return;
    
    std::vector<QString> names = battleSystem->getTeamNames();
    
    if (index >= (int)names.size()) {
        destroyPokemonMenu();
        
        battleSystem->returnToMainMenu();
        
        inPokemonMenu = false;
        inBattleMenu = true;
        battleMenuIndex = 0;
        
        setBattleText("What will " + battleSystem->getPlayerPokemonName() + " do?");
        startTextAnimation();
        
        updateBattleCursor();
        return;
    }
    
    int oldActiveIndex = battleSystem->getActivePokemonIndex();
    battleSystem->processPokemonAction(index);
    destroyPokemonMenu();
    
    inPokemonMenu = false;
    battleMenuIndex = 0;
    
    setBattleText(battleSystem->getLastMessage().isEmpty() ? "Go! " + battleSystem->getPlayerPokemonName() + "!" : battleSystem->getLastMessage());
    startTextAnimation();
    
    updateBattleUI();
    
    bool switchSuccessful = (battleSystem->getActivePokemonIndex() != oldActiveIndex);
    bool isErrorMessage = (fullBattleText.contains("already using") || fullBattleText.contains("fainted"));
    
    if (switchSuccessful && !isErrorMessage) {
        // Update player Pokemon sprite when switching
        if (gamePlayer && battlePlayerPokemonItem) {
            const Pokemon* newActive = gamePlayer->getActivePokemon();
            if (newActive) {
                QString playerSpritePath = QString::fromStdString(newActive->getBackSpritePath());
                QPixmap playerPx(playerSpritePath);
                if (playerPx.isNull()) {
                    QString spriteDir = QString::fromStdString(newActive->getSpriteDir());
                    playerSpritePath = ":/" + spriteDir + "/back.png";
                    playerPx = QPixmap(playerSpritePath);
                }
                if (playerPx.isNull()) {
                    playerPx = QPixmap(":/Battle/assets/pokemon_sprites/006_charizard/back.png");
                }
                if (!playerPx.isNull()) {
                    float sx = 120.0f / playerPx.width();
                    float sy = 120.0f / playerPx.height();
                    float scale = std::min(sx, sy);
                    battlePlayerPokemonItem->setPixmap(playerPx);
                    battlePlayerPokemonItem->setScale(scale);
                    qreal h = playerPx.height() * scale;
                    battlePlayerPokemonItem->setPos(100, 272 - h - 50);
                }
            }
        }
        
        inBattleMenu = false;
        QTimer::singleShot(1000, [=]() {
            enemyTurn();
        });
    } else {
        inBattleMenu = true;
        QTimer::singleShot(1000, [=]() {
            setBattleText("What will " + battleSystem->getPlayerPokemonName() + " do?");
            startTextAnimation();
            updateBattleCursor();
        });
    }
}

void BattleSequence::destroyMoveMenu()
{
    if (battleScene) {
        for (QGraphicsTextItem *t : moveMenuOptions) {
            if (t) {
                battleScene->removeItem(t);
                delete t;
            }
        }
    }
    moveMenuOptions.clear();

    if (moveMenuRect) {
        if (battleScene)
            battleScene->removeItem(moveMenuRect);
        delete moveMenuRect;
        moveMenuRect = nullptr;
    }
}

void BattleSequence::destroyBagMenu()
{
    if (battleScene) {
        for (QGraphicsTextItem *t : bagMenuOptions) {
            if (t) {
                battleScene->removeItem(t);
                delete t;
            }
        }
    }
    bagMenuOptions.clear();
    bagMenuItemIndices.clear();

    if (bagMenuRect) {
        if (battleScene)
            battleScene->removeItem(bagMenuRect);
        delete bagMenuRect;
        bagMenuRect = nullptr;
    }
}

void BattleSequence::destroyPokemonMenu()
{
    if (battleScene) {
        for (QGraphicsTextItem *t : pokemonMenuOptions) {
            if (t) {
                battleScene->removeItem(t);
                delete t;
            }
        }
    }
    pokemonMenuOptions.clear();

    if (pokemonMenuRect) {
        if (battleScene)
            battleScene->removeItem(pokemonMenuRect);
        delete pokemonMenuRect;
        pokemonMenuRect = nullptr;
    }
}

void BattleSequence::battleZoomReveal()
{
    animations.battleZoomReveal(this);
}

void BattleSequence::animateBattleEntrances()
{
    animations.animateBattleEntrances(this);
}

void BattleSequence::slideInCommandMenu()
{
    animations.slideInCommandMenu(this);
}

void BattleSequence::slideOutCommandMenu(std::function<void()> onFinished)
{
    animations.slideOutCommandMenu(this, onFinished);
}

void BattleSequence::fadeInBattleScreen()
{
    // Implementation if needed
}

void BattleSequence::fadeOutBattleScreen(std::function<void()> onFinished)
{
    if (!fadeEffect) {
        fadeEffect = new QGraphicsOpacityEffect();
        view->setGraphicsEffect(fadeEffect);
        fadeAnim = new QPropertyAnimation(fadeEffect, "opacity");
    }

    fadeAnim->stop();
    fadeAnim->setDuration(350);
    fadeAnim->setStartValue(1.0);
    fadeAnim->setEndValue(0.0);

    if (onFinished) {
        QObject::disconnect(fadeAnim, &QPropertyAnimation::finished, nullptr, nullptr);

        connect(fadeAnim, &QPropertyAnimation::finished, this, [=]() {
            fadeEffect->setOpacity(1.0);
            onFinished();
        });
    }

    fadeAnim->start();
}

void BattleSequence::attemptCatchPokemon(int itemIndex)
{
    if (!battleSystem || !battleSystem->getBattle() || !gamePlayer) return;
    
    // Only allow catching in wild battles
    if (!battleSystem->getBattle()->getIsWildBattle()) {
        setBattleText("You can't catch another trainer's Pokemon!");
        startTextAnimation();
        destroyBagMenu();
        inBagMenu = false;
        inBattleMenu = true;
        battleMenuIndex = 0;
        QTimer::singleShot(1500, [=]() {
            updateBattleCursor();
        });
        return;
    }
    
    const auto& items = gamePlayer->getBag().getItems();
    if (itemIndex < 0 || itemIndex >= static_cast<int>(items.size())) return;
    
    Item& pokeball = const_cast<Item&>(items[itemIndex]);
    if (pokeball.getQuantity() <= 0) return;
    
    // Use the Pokeball
    pokeball.use();
    
    // Get enemy Pokemon
    const Pokemon* enemyPokemon = enemyPlayer ? enemyPlayer->getActivePokemon() : nullptr;
    if (!enemyPokemon) return;
    
    // Calculate catch rate (simplified Gen 1 formula)
    // Base catch rate depends on Pokemon species (simplified to use HP)
    int enemyHP = enemyPokemon->getCurrentHP();
    int enemyMaxHP = enemyPokemon->getMaxHP();
    float hpPercent = static_cast<float>(enemyHP) / static_cast<float>(enemyMaxHP);
    
    // Catch rate formula (simplified): lower HP = easier to catch
    // Base catch rate: 255 (max), modified by HP percentage
    int catchRate = 255;
    if (hpPercent > 0.5f) {
        catchRate = static_cast<int>(255 * (1.0f - hpPercent));
    }
    
    // Roll for catch (0-255, need to be <= catchRate)
    int roll = QRandomGenerator::global()->bounded(256);
    bool caught = (roll <= catchRate);
    
    if (caught) {
        // Successfully caught!
        Pokemon caughtPokemon(enemyPokemon->getDexNumber(), enemyPokemon->getLevel());
        // Pokemon is created with full HP by default, so no need to heal
        
        // Add to team if there's space, otherwise to PC (for now, just add if space)
        if (gamePlayer->getTeam().size() < 6) {
            gamePlayer->addPokemon(caughtPokemon);
            setBattleText("Gotcha! " + QString::fromStdString(caughtPokemon.getName()) + " was caught!");
        } else {
            setBattleText("Gotcha! " + QString::fromStdString(caughtPokemon.getName()) + " was caught!\n(Team full - sent to PC)");
        }
        
        startTextAnimation();
        destroyBagMenu();
        inBagMenu = false;
        inBattleMenu = false;
        
        // End battle
        QTimer::singleShot(2000, [=]() {
            fadeOutBattleScreen([=]() {
                closeBattle();
            });
        });
    } else {
        // Failed to catch
        setBattleText("The wild " + QString::fromStdString(enemyPokemon->getName()) + " broke free!");
        startTextAnimation();
        destroyBagMenu();
        inBagMenu = false;
        inBattleMenu = false;
        battleMenuIndex = 0;
        
        // Enemy gets a turn
        QTimer::singleShot(1500, [=]() {
            enemyTurn();
        });
    }
}

bool BattleSequence::checkAndAutoSwitchPokemon()
{
    if (!battleSystem || !gamePlayer) return false;
    
    const Pokemon* activePokemon = gamePlayer->getActivePokemon();
    if (!activePokemon || !activePokemon->isFainted()) {
        return false; // Pokemon is not fainted, no need to switch
    }
    
    // Find next surviving Pokemon
    std::vector<int> usableIndices = gamePlayer->getUsablePokemonIndices();
    
    if (usableIndices.empty()) {
        // No Pokemon available - game over
        return false;
    }
    
    // Switch to the first available Pokemon
    int nextIndex = usableIndices[0];
    battleSystem->processPokemonAction(nextIndex);
    
    // Update sprite
    if (battlePlayerPokemonItem) {
        const Pokemon* newActive = gamePlayer->getActivePokemon();
        if (newActive) {
            QString playerSpritePath = QString::fromStdString(newActive->getBackSpritePath());
            QPixmap playerPx(playerSpritePath);
            if (playerPx.isNull()) {
                QString spriteDir = QString::fromStdString(newActive->getSpriteDir());
                playerSpritePath = ":/" + spriteDir + "/back.png";
                playerPx = QPixmap(playerSpritePath);
            }
            if (playerPx.isNull()) {
                playerPx = QPixmap(":/Battle/assets/pokemon_sprites/006_charizard/back.png");
            }
            if (!playerPx.isNull()) {
                float sx = 120.0f / playerPx.width();
                float sy = 120.0f / playerPx.height();
                float scale = std::min(sx, sy);
                battlePlayerPokemonItem->setPixmap(playerPx);
                battlePlayerPokemonItem->setScale(scale);
                qreal h = playerPx.height() * scale;
                battlePlayerPokemonItem->setPos(100, 272 - h - 50);
            }
        }
    }
    
    updateBattleUI();
    return true; // Successfully switched
}


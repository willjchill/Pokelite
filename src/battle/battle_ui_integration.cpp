// battle_ui_integration.cpp
// UI INTEGRATION CODE - Uses real BattleSystem instead of dummy logic

#include "mainwindow.h"
#include "../game_logic/PokemonData.h"
#include <QDebug>
#include <QBrush>
#include <QPen>
#include <QTimer>
#include <QRandomGenerator>

// Separate MOVE MENU UI (white box + text)
static QGraphicsRectItem *moveMenuRect = nullptr;
static QVector<QGraphicsTextItem*> moveMenuOptions;

// BAG MENU UI
static QGraphicsRectItem *bagMenuRect = nullptr;
static QVector<QGraphicsTextItem*> bagMenuOptions;

// POKEMON MENU UI
static QGraphicsRectItem *pokemonMenuRect = nullptr;
static QVector<QGraphicsTextItem*> pokemonMenuOptions;


// Helper to destroy the move menu cleanly
static void destroyMoveMenu(QGraphicsScene *scene)
{
    if (scene) {
        for (QGraphicsTextItem *t : moveMenuOptions) {
            if (t) {
                scene->removeItem(t);
                delete t;
            }
        }
    }
    moveMenuOptions.clear();

    if (moveMenuRect) {
        if (scene)
            scene->removeItem(moveMenuRect);
        delete moveMenuRect;
        moveMenuRect = nullptr;
    }
}

// Helper to destroy the bag menu
static void destroyBagMenu(QGraphicsScene *scene)
{
    if (scene) {
        for (QGraphicsTextItem *t : bagMenuOptions) {
            if (t) {
                scene->removeItem(t);
                delete t;
            }
        }
    }
    bagMenuOptions.clear();

    if (bagMenuRect) {
        if (scene)
            scene->removeItem(bagMenuRect);
        delete bagMenuRect;
        bagMenuRect = nullptr;
    }
}

// Helper to destroy the pokemon menu
static void destroyPokemonMenu(QGraphicsScene *scene)
{
    if (scene) {
        for (QGraphicsTextItem *t : pokemonMenuOptions) {
            if (t) {
                scene->removeItem(t);
                delete t;
            }
        }
    }
    pokemonMenuOptions.clear();

    if (pokemonMenuRect) {
        if (scene)
            scene->removeItem(pokemonMenuRect);
        delete pokemonMenuRect;
        pokemonMenuRect = nullptr;
    }
}


// ======================================================
// ===================== BATTLE START ===================
// ======================================================

void MainWindow::tryWildEncounter()
{
    // Initialize Pokemon data if not already done
    initializePokemonDataFromJSON();
    
    // Create player if not exists
    if (!gamePlayer) {
        gamePlayer = new Player("Player", PlayerType::HUMAN);
        // Add a starter Pokemon (Bulbasaur, level 5)
        Pokemon starter(1, 5);
        gamePlayer->addPokemon(starter);
    }
    
    // Create enemy player (wild Pokemon)
    if (enemyPlayer) {
        delete enemyPlayer;
    }
    enemyPlayer = new Player("Wild Pokemon", PlayerType::NPC);
    // Random wild Pokemon (for now, use Charizard as example)
    int randomDex = QRandomGenerator::global()->bounded(1, 152); // 1-151
    Pokemon wildPokemon(randomDex, 5);
    enemyPlayer->addPokemon(wildPokemon);
    
    // Initialize battle system
    if (battleSystem) {
        delete battleSystem;
    }
    battleSystem = new BattleSystem();
    battleSystem->initializeBattle(gamePlayer, enemyPlayer, true);
    
    // Remember overworld scene so we can return later
    overworldScene = scene;

    // Reset state
    inBattle        = true;
    inBattleMenu    = true;
    inBagMenu       = false;
    inPokemonMenu   = false;
    battleMenuIndex = 0;
    fullBattleText.clear();
    battleTextIndex = 0;

    // Clean any leftover menus from previous battle
    destroyMoveMenu(battleScene);
    destroyBagMenu(battleScene);
    destroyPokemonMenu(battleScene);

    // New battle scene
    battleScene = new QGraphicsScene(0, 0, 480, 272, this);

    // Background
    QPixmap battleBg(":/assets/battle/battle_bg.png");
    if (battleBg.isNull()) {
        battleBg = QPixmap(480, 272);
        battleBg.fill(Qt::black);
    }
    battleScene->addPixmap(battleBg)->setZValue(0);

    // Player trainer sprite (background/backdrop)
    QPixmap trainerPx(":/assets/battle/trainer.png");
    if (!trainerPx.isNull()) {
        float sx = 120.0f / trainerPx.width();
        float sy = 120.0f / trainerPx.height();
        float scale = std::min(sx, sy);

        battleTrainerItem = battleScene->addPixmap(trainerPx);
        battleTrainerItem->setScale(scale);

        qreal h = trainerPx.height() * scale;
        battleTrainerItem->setPos(20, 272 - h - 60);
        battleTrainerItem->setZValue(0);  // Behind Pokemon
    }

    // Player's Pokemon sprite - use back.png from sprite directory
    const Pokemon* playerPokemon = gamePlayer->getActivePokemon();
    if (playerPokemon) {
        QString playerSpritePath = QString::fromStdString(playerPokemon->getBackSpritePath());
        qDebug() << "Player Pokemon:" << QString::fromStdString(playerPokemon->getName()) 
                 << "Sprite path:" << playerSpritePath;
        QPixmap playerPx(playerSpritePath);
        if (playerPx.isNull()) {
            // Fallback: try constructing path manually
            QString spriteDir = QString::fromStdString(playerPokemon->getSpriteDir());
            playerSpritePath = ":/" + spriteDir + "/back.png";
            qDebug() << "Trying alternative path:" << playerSpritePath;
            playerPx = QPixmap(playerSpritePath);
        }
        if (playerPx.isNull()) {
            // Fallback to charizard if sprite not found
            qDebug() << "Sprite not found, using charizard fallback";
            playerPx = QPixmap(":/game_logic/sprites/006_charizard/back.png");
        }
        if (!playerPx.isNull()) {
            float sx = 120.0f / playerPx.width();
            float sy = 120.0f / playerPx.height();
            float scale = std::min(sx, sy);

            battlePlayerPokemonItem = battleScene->addPixmap(playerPx);
            battlePlayerPokemonItem->setScale(scale);

            qreal h = playerPx.height() * scale;
            // Position player's Pokemon to the right of trainer, not covering it
            battlePlayerPokemonItem->setPos(100, 272 - h - 50);
            battlePlayerPokemonItem->setZValue(2);  // In front of trainer
        }
    }

    // Enemy Pokémon sprite - use front.png from sprite directory
    const Pokemon* enemyPokemon = enemyPlayer->getActivePokemon();
    QString enemySpritePath = ":/game_logic/sprites/006_charizard/front.png";  // Default fallback
    if (enemyPokemon) {
        enemySpritePath = QString::fromStdString(enemyPokemon->getFrontSpritePath());
        qDebug() << "Enemy Pokemon:" << QString::fromStdString(enemyPokemon->getName()) 
                 << "Sprite path:" << enemySpritePath;
    }
    QPixmap enemyPx(enemySpritePath);
    if (enemyPx.isNull()) {
        // Fallback: try constructing path manually
        if (enemyPokemon) {
            QString spriteDir = QString::fromStdString(enemyPokemon->getSpriteDir());
            enemySpritePath = ":/" + spriteDir + "/front.png";
            qDebug() << "Trying alternative path:" << enemySpritePath;
            enemyPx = QPixmap(enemySpritePath);
        }
        if (enemyPx.isNull()) {
            // Fallback to charizard if sprite not found
            qDebug() << "Sprite not found, using charizard fallback";
            enemyPx = QPixmap(":/game_logic/sprites/006_charizard/front.png");
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

    // HP bars, dialogue box, command box, etc.
    setupBattleUI();

    // Switch to battle scene
    view->setScene(battleScene);
    view->setFixedSize(480, 272);
    setFixedSize(480, 272);

    setFocus();
    view->setFocus();

    // Start the battle
    battleSystem->startBattle();
    
    // Update UI with initial state
    updateBattleUI();

    // Animations from battle_animations.cpp
    battleZoomReveal();
    animateBattleEntrances();
    slideInCommandMenu();
}

// ======================================================
// ===================== UI SETUP =======================
// ======================================================

void MainWindow::setupBattleUI()
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

    fullBattleText   = "What will " + (battleSystem ? battleSystem->getPlayerPokemonName() : "BULBASAUR") + " do?";
    battleTextIndex  = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(30);

    // Command box (graphics only, text is baked into the image)
    QPixmap cmdBox(":/assets/battle/ui/command_box.png");
    commandBoxSprite = battleScene->addPixmap(cmdBox);
    commandBoxSprite->setPos(480 - 160 - 50, 272 - 64 - 17);
    commandBoxSprite->setScale(1.3);
    commandBoxSprite->setZValue(2);

    // Invisible anchors for the 4 command slots (FIGHT/BAG/POKEMON/RUN)
    int px[4] = {25, 100, 25, 100};
    int py[4] = {18, 18, 42, 42};

    battleMenuOptions.clear();
    for (int i = 0; i < 4; ++i) {
        QGraphicsTextItem *t = new QGraphicsTextItem(QString::number(i));
        t->setVisible(false);  // IMPORTANT: no visible letters in command box
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

    // HP bars start full (will be updated by updateBattleUI)
    updateBattleUI();
}

// ======================================================
// ================== INPUT HANDLING ====================
// ======================================================

void MainWindow::handleBattleKey(QKeyEvent *event)
{
    if (!inBattle || !battleSystem)
        return;

    // Ignore input during enemy turn
    if (battleSystem->isWaitingForEnemyTurn())
        return;

    bool inMoveMenu = battleSystem->isWaitingForPlayerMove();
    
    // Determine which menu we're in and get max index
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

    // ---- CURSOR MOVEMENT (arrows + WASD) ----
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
        // Confirm selection
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

    // ---- UPDATE CURSOR ----
    updateBattleCursor();
}

// ======================================================
// =============== CURSOR / COLOR HELPERS ===============
// ======================================================

void MainWindow::updateBattleCursor()
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

// Helper function to capitalize first letter
static QString capitalizeFirst(const QString& str) {
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1).toLower();
}

// ======================================================
// ============ UPDATE UI FROM BATTLE STATE =============
// ======================================================

void MainWindow::updateBattleUI()
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
    
    if (playerMaxHP > 0) {
        float playerHpPercent = (float)playerHP / (float)playerMaxHP;
        playerHpFill->setRect(0, 0, 86 * playerHpPercent, 6);
        setHpColor(playerHpFill, playerHpPercent);
    }
    
    if (enemyMaxHP > 0) {
        float enemyHpPercent = (float)enemyHP / (float)enemyMaxHP;
        enemyHpFill->setRect(0, 0, 62 * enemyHpPercent, 6);
        setHpColor(enemyHpFill, enemyHpPercent);
    }
}

// ======================================================
// ============ BATTLE FLOW: COMMAND MENU ===============
// ======================================================

void MainWindow::playerSelectedOption(int index)
{
    if (!battleSystem) return;
    
    // 0 = FIGHT
    if (index == 0) {
        battleSystem->processAction(BattleAction::FIGHT);
        
        // Clean up any other menus
        destroyBagMenu(battleScene);
        destroyPokemonMenu(battleScene);
        
        // Build move menu
        destroyMoveMenu(battleScene);
        
        std::vector<QString> moves = battleSystem->getPlayerMoves();
        std::vector<int> pp = battleSystem->getPlayerMovePP();
        std::vector<int> maxPP = battleSystem->getPlayerMoveMaxPP();
        
        QFont f("Pokemon Fire Red", 9, QFont::Bold);
        
        // Make box bigger to accommodate longer move names and PP text
        const qreal boxW = 240, boxH = 80;
        qreal boxX = dialogueBoxSprite->pos().x() + 10;
        qreal boxY = dialogueBoxSprite->pos().y() - boxH - 4;
        
        moveMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
        moveMenuRect->setBrush(Qt::white);
        moveMenuRect->setPen(QPen(Qt::black, 2));
        moveMenuRect->setZValue(3);
        battleScene->addItem(moveMenuRect);
        
        moveMenuOptions.clear();
        int numMoves = std::min(4, (int)moves.size() + 1); // +1 for BACK
        for (int i = 0; i < numMoves; ++i) {
            QString label;
            if (i < (int)moves.size()) {
                // Truncate long move names and format PP nicely
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
            // Better spacing to prevent overlap
            t->setPos(boxX + 12 + col * 115,
                      boxY + 10 + row * 35);
            t->setZValue(4);
            
            battleScene->addItem(t);
            moveMenuOptions.push_back(t);
        }
        
        // Enter MOVE MENU mode
        inBattleMenu = false;
        battleMenuIndex = 0;
        
        fullBattleText = battleSystem->getLastMessage();
        if (fullBattleText.isEmpty()) {
            fullBattleText = "Choose a move!";
        }
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);
        
        updateBattleCursor();
        return;
    }
    
    // 1 = BAG
    if (index == 1) {
        battleSystem->processAction(BattleAction::BAG);
        showBagMenu();
        return;
    }
    
    // 2 = POKEMON
    if (index == 2) {
        battleSystem->processAction(BattleAction::POKEMON);
        showPokemonMenu();
        return;
    }
    
    // 3 = RUN
    if (index == 3) {
        // Process run action directly to get the result
        battleSystem->processRunAction();
        
        // Check if run was successful
        bool runSuccessful = battleSystem->isBattleOver();
        
        // Get the message (should be set by processRunAction)
        fullBattleText = battleSystem->getLastMessage();
        if (fullBattleText.isEmpty()) {
            fullBattleText = runSuccessful ? "Got away safely!" : "Can't escape!";
        }
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);
        
        inBattleMenu = false;
        
        if (runSuccessful) {
            // Run was successful - close the battle
            QTimer::singleShot(1000, [=]() {
                fadeOutBattleScreen([=]() {
                    closeBattleReturnToMap();
                });
            });
        } else {
            // Run failed - enemy gets a turn, then continue battle
            QTimer::singleShot(1000, [=]() {
                enemyTurn();
            });
        }
        return;
    }
}

// ======================================================
// ============ BATTLE FLOW: MOVE MENU ==================
// ======================================================

void MainWindow::playerChoseMove(int moveIndex)
{
    if (!battleSystem) return;
    
    std::vector<QString> moves = battleSystem->getPlayerMoves();
    
    // BACK option is last
    if (moveIndex >= (int)moves.size()) {
        destroyMoveMenu(battleScene);
        destroyBagMenu(battleScene);
        destroyPokemonMenu(battleScene);
        
        // Reset battle state back to main menu
        battleSystem->returnToMainMenu();
        
        inBattleMenu = true;
        inBagMenu = false;
        inPokemonMenu = false;
        battleMenuIndex = 0;
        
        fullBattleText = "What will " + battleSystem->getPlayerPokemonName() + " do?";
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);
        
        updateBattleCursor();
        return;
    }
    
    // Real move chosen
    QString moveName = moves[moveIndex];
    fullBattleText = battleSystem->getPlayerPokemonName() + " used " + moveName + "!";
    battleTextIndex = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(22);
    
    destroyMoveMenu(battleScene);
    inBattleMenu = false;
    
    // Process the move (this executes the full turn including enemy's move)
    battleSystem->processFightAction(moveIndex);
    
    // Update UI after the turn
    updateBattleUI();
    
    // Check if battle ended (after the full turn)
    if (battleSystem->isBattleOver()) {
        Player* winner = battleSystem->getWinner();
        if (winner == gamePlayer) {
            fullBattleText = "The foe fainted!";
        } else {
            fullBattleText = battleSystem->getPlayerPokemonName() + " fainted...";
        }
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);
        
        QTimer::singleShot(1200, [=]() {
            fadeOutBattleScreen([=]() {
                closeBattleReturnToMap();
            });
        });
        return;
    }
    
    // Show enemy turn message (the turn was already executed, this is just for display)
    QTimer::singleShot(1000, [=]() {
        enemyTurn();
    });
}

// ======================================================
// ============== BATTLE FLOW: ENEMY TURN ===============
// ======================================================

void MainWindow::enemyTurn()
{
    if (!battleSystem) return;
    
    // Note: The enemy's move was already executed by Battle::processFightAction
    // This function just updates the UI to show the result
    
    // Get enemy Pokemon name for display
    QString enemyName = battleSystem->getEnemyPokemonName();
    if (enemyName.isEmpty()) {
        enemyName = "The foe";
    }
    
    fullBattleText = enemyName + " used a move!";
    battleTextIndex = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(22);
    
    // Update UI with current HP after the turn
    updateBattleUI();
    
    // Check if battle ended
    if (battleSystem->isBattleOver()) {
        Player* winner = battleSystem->getWinner();
        if (winner == gamePlayer) {
            fullBattleText = "The foe fainted!";
        } else {
            fullBattleText = battleSystem->getPlayerPokemonName() + " fainted...";
        }
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);
        
        inBattleMenu = false;
        
        QTimer::singleShot(1400, [=]() {
            fadeOutBattleScreen([=]() {
                closeBattleReturnToMap();
            });
        });
        return;
    }
    
    // Otherwise, go back to main command menu
    QTimer::singleShot(900, [=]() {
        fullBattleText = "What will " + battleSystem->getPlayerPokemonName() + " do?";
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);
        
        inBattleMenu = true;
        battleMenuIndex = 0;
        
        destroyMoveMenu(battleScene);
        updateBattleCursor();
    });
}

// ======================================================
// ================== CLOSE BATTLE =======================
// ======================================================

void MainWindow::closeBattleReturnToMap()
{
    if (battleSystem) {
        delete battleSystem;
        battleSystem = nullptr;
    }
    
    if (enemyPlayer) {
        delete enemyPlayer;
        enemyPlayer = nullptr;
    }
    
    inBattle = false;
    inBattleMenu = false;
    inBagMenu = false;
    inPokemonMenu = false;
    
    // Return to overworld
    if (overworldScene) {
        view->setScene(overworldScene);
        scene = overworldScene;
        overworldScene = nullptr;
    }
    
    // restore opacity
    if (fadeEffect)
        fadeEffect->setOpacity(1.0);
    
    // re-center camera (note: player is QGraphicsPixmapItem*, not Player*)
    if (player)
        view->centerOn(player);
    
    // force redraw so screen looks EXACTLY like before battle
    view->viewport()->update();
    if (overworldScene)
        overworldScene->update();
    
    setFocus();
    view->setFocus();
}

// ======================================================
// ================== FADE OUT WRAPPER ==================
// ======================================================

void MainWindow::fadeOutBattleScreen(std::function<void()> onFinished)
{
    if (!fadeEffect) {
        fadeEffect = new QGraphicsOpacityEffect(this);
        view->setGraphicsEffect(fadeEffect);
        fadeAnim = new QPropertyAnimation(fadeEffect, "opacity");
    }

    fadeAnim->stop();
    fadeAnim->setDuration(350);
    fadeAnim->setStartValue(1.0);
    fadeAnim->setEndValue(0.0);

    if (onFinished) {
        // IMPORTANT FIX: restore opacity after fade
        QObject::disconnect(fadeAnim, &QPropertyAnimation::finished, nullptr, nullptr);

        connect(fadeAnim, &QPropertyAnimation::finished, this, [=]() {
            fadeEffect->setOpacity(1.0);   // <<< THIS IS THE FIX
            onFinished();
        });
    }

    fadeAnim->start();
}

// ======================================================
// ============ BAG MENU UI ===================
// ======================================================

void MainWindow::showBagMenu()
{
    if (!battleSystem || !battleScene) return;
    
    // Clean up other menus
    destroyMoveMenu(battleScene);
    destroyPokemonMenu(battleScene);
    
    std::vector<QString> items = battleSystem->getBagItems();
    std::vector<int> quantities = battleSystem->getBagItemQuantities();
    
    QFont f("Pokemon Fire Red", 9, QFont::Bold);
    
    // Create menu box
    const qreal boxW = 240, boxH = 120;
    qreal boxX = dialogueBoxSprite->pos().x() + 10;
    qreal boxY = dialogueBoxSprite->pos().y() - boxH - 4;
    
    bagMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    bagMenuRect->setBrush(Qt::white);
    bagMenuRect->setPen(QPen(Qt::black, 2));
    bagMenuRect->setZValue(3);
    battleScene->addItem(bagMenuRect);
    
    bagMenuOptions.clear();
    int numItems = std::min(6, (int)items.size() + 1); // +1 for BACK
    for (int i = 0; i < numItems; ++i) {
        QString label;
        if (i < (int)items.size()) {
            QString itemName = items[i];
            if (itemName.length() > 15) {
                itemName = itemName.left(13) + "...";
            }
            label = itemName + " x" + QString::number(quantities[i]);
        } else {
            label = "BACK";
        }
        
        QGraphicsTextItem *t = new QGraphicsTextItem(label);
        t->setFont(f);
        t->setDefaultTextColor(Qt::black);
        
        int row = i / 2;
        int col = i % 2;
        t->setPos(boxX + 12 + col * 115,
                  boxY + 10 + row * 20);
        t->setZValue(4);
        
        battleScene->addItem(t);
        bagMenuOptions.push_back(t);
    }
    
    inBagMenu = true;
    inBattleMenu = false;
    inPokemonMenu = false;
    battleMenuIndex = 0;
    
    fullBattleText = "Choose an item!";
    battleTextIndex = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(22);
    
    updateBattleCursor();
}

// ======================================================
// ============ POKEMON MENU UI ===================
// ======================================================

void MainWindow::showPokemonMenu()
{
    if (!battleSystem || !battleScene) return;
    
    // Clean up other menus
    destroyMoveMenu(battleScene);
    destroyBagMenu(battleScene);
    
    std::vector<QString> names = battleSystem->getTeamNames();
    std::vector<int> hp = battleSystem->getTeamHP();
    std::vector<int> maxHP = battleSystem->getTeamMaxHP();
    std::vector<int> levels = battleSystem->getTeamLevels();
    std::vector<bool> fainted = battleSystem->getTeamFainted();
    int activeIndex = battleSystem->getActivePokemonIndex();
    
    QFont f("Pokemon Fire Red", 9, QFont::Bold);
    
    // Create menu box - make it bigger to fit more info
    const qreal boxW = 240, boxH = 140;
    qreal boxX = dialogueBoxSprite->pos().x() + 10;
    qreal boxY = dialogueBoxSprite->pos().y() - boxH - 4;
    
    pokemonMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    pokemonMenuRect->setBrush(Qt::white);
    pokemonMenuRect->setPen(QPen(Qt::black, 2));
    pokemonMenuRect->setZValue(3);
    battleScene->addItem(pokemonMenuRect);
    
    pokemonMenuOptions.clear();
    int numPokemon = std::min(6, (int)names.size() + 1); // +1 for BACK
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
        t->setDefaultTextColor(Qt::black);
        
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
    
    fullBattleText = "Choose a Pokemon!";
    battleTextIndex = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(22);
    
    updateBattleCursor();
}


// ======================================================
// ============ BAG/POKEMON/SUBMENU HANDLERS ===================
// ======================================================

void MainWindow::playerSelectedBagItem(int index)
{
    if (!battleSystem) return;
    
    std::vector<QString> items = battleSystem->getBagItems();
    
    // BACK option is last
    if (index >= (int)items.size()) {
        destroyBagMenu(battleScene);
        
        battleSystem->returnToMainMenu();
        
        inBagMenu = false;
        inBattleMenu = true;
        battleMenuIndex = 0;
        
        fullBattleText = "What will " + battleSystem->getPlayerPokemonName() + " do?";
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);
        
        updateBattleCursor();
        return;
    }
    
    // Use item
    battleSystem->processBagAction(index);
    destroyBagMenu(battleScene);
    
    inBagMenu = false;
    inBattleMenu = false;
    battleMenuIndex = 0;
    
    fullBattleText = battleSystem->getLastMessage();
    if (fullBattleText.isEmpty()) {
        fullBattleText = "Used item!";
    }
    battleTextIndex = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(22);
    
    updateBattleUI();
    
    // Enemy gets a turn after using item
    QTimer::singleShot(1000, [=]() {
        enemyTurn();
    });
}

void MainWindow::playerSelectedPokemon(int index)
{
    if (!battleSystem) return;
    
    std::vector<QString> names = battleSystem->getTeamNames();
    
    // BACK option is last
    if (index >= (int)names.size()) {
        destroyPokemonMenu(battleScene);
        
        battleSystem->returnToMainMenu();
        
        inPokemonMenu = false;
        inBattleMenu = true;
        battleMenuIndex = 0;
        
        fullBattleText = "What will " + battleSystem->getPlayerPokemonName() + " do?";
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);
        
        updateBattleCursor();
        return;
    }
    
    // Switch Pokemon
    int oldActiveIndex = battleSystem->getActivePokemonIndex();
    battleSystem->processPokemonAction(index);
    destroyPokemonMenu(battleScene);
    
    inPokemonMenu = false;
    battleMenuIndex = 0;
    
    fullBattleText = battleSystem->getLastMessage();
    if (fullBattleText.isEmpty()) {
        fullBattleText = "Go! " + battleSystem->getPlayerPokemonName() + "!";
    }
    battleTextIndex = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(22);
    
    updateBattleUI();
    
    // Check if switch was successful (active index changed) or if it was an error message
    bool switchSuccessful = (battleSystem->getActivePokemonIndex() != oldActiveIndex);
    bool isErrorMessage = (fullBattleText.contains("already using") || fullBattleText.contains("fainted"));
    
    if (switchSuccessful && !isErrorMessage) {
        // Switch was successful - enemy gets a turn
        inBattleMenu = false;
        QTimer::singleShot(1000, [=]() {
            enemyTurn();
        });
    } else {
        // Switch failed or error - return to menu without enemy turn
        inBattleMenu = true;
        QTimer::singleShot(1000, [=]() {
            fullBattleText = "What will " + battleSystem->getPlayerPokemonName() + " do?";
            battleTextIndex = 0;
            battleTextItem->setPlainText("");
            battleTextTimer.start(22);
            updateBattleCursor();
        });
    }
}


// ======================================================
// ================== HP COLOR HELPER ===================
// ======================================================

void MainWindow::setHpColor(QGraphicsRectItem *hpBar, float hpPercent)
{
    if (hpPercent > 0.5f)       hpBar->setBrush(QColor("#3CD75F"));
    else if (hpPercent > 0.2f)  hpBar->setBrush(QColor("#FFCB43"));
    else                        hpBar->setBrush(QColor("#FF4949"));
}


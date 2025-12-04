#include "mainwindow.h"
#include "overworld.h"
#include "battle_sequence.h"
#include <QDebug>
#include <QApplication>
#include <QShowEvent>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), gamepadThread(nullptr)
{
    // Set window size
    setFixedSize(480, 272);

    // Create scene and view
    scene = new QGraphicsScene(0, 0, 480, 272, this);
    view = new QGraphicsView(scene, this);
    view->setFixedSize(480, 272);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setFocusPolicy(Qt::StrongFocus);

    setCentralWidget(view);
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    view->setFocus();

    // Initialize game systems
    overworld = new Overworld(scene, view);
    battleSequence = new BattleSequence(scene, view);

    // Connect overworld signals
    connect(overworld, &Overworld::wildEncounterTriggered, this, &MainWindow::onWildEncounterTriggered);
    
    // Connect battle sequence signals
    connect(battleSequence, &BattleSequence::battleEnded, this, &MainWindow::onBattleEnded);

    // Initialize player
    initializePlayer();

    // Load default map
    overworld->loadMap("route1");
}

MainWindow::~MainWindow()
{
    if (gamepadThread) {
        gamepadThread->stop();
        delete gamepadThread;
    }
    
    if (battleSequence) {
        delete battleSequence;
    }
    
    if (overworld) {
        delete overworld;
    }
    
    if (battleSystem) {
        delete battleSystem;
    }
    
    if (gamePlayer) {
        delete gamePlayer;
    }
    
    if (enemyPlayer) {
        delete enemyPlayer;
    }
}

void MainWindow::initializePlayer()
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
    
    // Set player in overworld
    overworld->setPlayer(gamePlayer);
}

void MainWindow::onWildEncounterTriggered()
{
    if (inBattle) return; // Already in battle
    
    startWildEncounter();
}

void MainWindow::startWildEncounter()
{
    // Clean up previous enemy player if exists
    if (enemyPlayer) {
        delete enemyPlayer;
    }
    
    // Create enemy player (wild Pokemon)
    enemyPlayer = new Player("Wild Pokemon", PlayerType::NPC);
    // Random wild Pokemon (for now, use random dex number)
    int randomDex = QRandomGenerator::global()->bounded(1, 152); // 1-151
    Pokemon wildPokemon(randomDex, 5);
    enemyPlayer->addPokemon(wildPokemon);
    
    // Initialize battle system
    if (battleSystem) {
        delete battleSystem;
    }
    battleSystem = new BattleSystem();
    battleSystem->initializeBattle(gamePlayer, enemyPlayer, true);
    
    // Start battle sequence
    battleSequence->startBattle(gamePlayer, enemyPlayer, battleSystem);
    
    inBattle = true;
}

void MainWindow::onBattleEnded()
{
    inBattle = false;
    
    // Clean up battle system and enemy player
    if (battleSystem) {
        delete battleSystem;
        battleSystem = nullptr;
    }
    
    if (enemyPlayer) {
        delete enemyPlayer;
        enemyPlayer = nullptr;
    }
    
    // Return to overworld scene
    view->setScene(scene);
    view->setFixedSize(480, 272);
    
    // Re-center camera on player
    QGraphicsPixmapItem* playerItem = overworld->getPlayerItem();
    if (playerItem) {
        view->centerOn(playerItem);
    }
    
    // Clean up battle scene
    QGraphicsScene* bs = battleSequence->getScene();
    if (bs && bs != scene) {
        delete bs;
    }
    
    setFocus();
    view->setFocus();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Handle gamepad input simulation
    if (event->key() == Qt::Key_M && !inBattle) {
        overworld->showOverworldMenu();
        return;
    }

    if (inBattle) {
        battleSequence->handleBattleKey(event);
        return;
    }
    
    // Handle overworld menu
    if (overworld->isInMenu()) {
        overworld->handleOverworldMenuKey(event);
        return;
    }
    
    // Handle overworld movement
    overworld->handleMovementInput(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    // Ignore auto-repeat releases so walking animation keeps running
    if (event->isAutoRepeat())
        return;

    if (!inBattle) {
        overworld->handleKeyRelease(event);
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // Ensure focus when window is shown
    QTimer::singleShot(0, this, [this]() {
        setFocus();
        view->setFocus();
        activateWindow();
    });
}

void MainWindow::handleGamepadInput(int type, int code, int value)
{
    // Linux input event types
    // EV_KEY = 1 (button events)
    // EV_ABS = 3 (analog stick/dpad events)
    
    if (type == 1) { // EV_KEY - button press/release
        bool pressed = (value == 1);
        
        // Map Xbox controller buttons to keyboard keys
        // BTN_A = 304, BTN_B = 305, BTN_X = 306, BTN_Y = 307
        // BTN_TL = 310, BTN_TR = 311
        // BTN_SELECT = 314, BTN_START = 315
        
        if (code == 304) { // A button
            if (pressed) {
                simulateKeyPress(Qt::Key_Return);
            } else {
                simulateKeyRelease(Qt::Key_Return);
            }
        } else if (code == 305) { // B button
            if (pressed) {
                simulateKeyPress(Qt::Key_Escape);
            } else {
                simulateKeyRelease(Qt::Key_Escape);
            }
        }
    } else if (type == 3) { // EV_ABS - analog stick/dpad
        // ABS_X = 0 (left stick X or dpad left/right)
        // ABS_Y = 1 (left stick Y or dpad up/down)
        // ABS_HAT0X = 16 (dpad X)
        // ABS_HAT0Y = 17 (dpad Y)
        
        if (code == 16) { // D-pad X
            if (value == -1) { // Left
                simulateKeyPress(Qt::Key_A);
            } else if (value == 1) { // Right
                simulateKeyPress(Qt::Key_D);
            } else { // Released
                simulateKeyRelease(Qt::Key_A);
                simulateKeyRelease(Qt::Key_D);
            }
        } else if (code == 17) { // D-pad Y
            if (value == -1) { // Up
                simulateKeyPress(Qt::Key_W);
            } else if (value == 1) { // Down
                simulateKeyPress(Qt::Key_S);
            } else { // Released
                simulateKeyRelease(Qt::Key_W);
                simulateKeyRelease(Qt::Key_S);
            }
        } else if (code == 0) { // Left stick X
            if (value < -10000) { // Left threshold
                simulateKeyPress(Qt::Key_A);
            } else if (value > 10000) { // Right threshold
                simulateKeyPress(Qt::Key_D);
            } else { // Center
                simulateKeyRelease(Qt::Key_A);
                simulateKeyRelease(Qt::Key_D);
            }
        } else if (code == 1) { // Left stick Y
            if (value < -10000) { // Up threshold
                simulateKeyPress(Qt::Key_W);
            } else if (value > 10000) { // Down threshold
                simulateKeyPress(Qt::Key_S);
            } else { // Center
                simulateKeyRelease(Qt::Key_W);
                simulateKeyRelease(Qt::Key_S);
            }
        }
    }
}

void MainWindow::simulateKeyPress(Qt::Key key)
{
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier);
    QApplication::postEvent(this, event);
}

void MainWindow::simulateKeyRelease(Qt::Key key)
{
    QKeyEvent *event = new QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier);
    QApplication::postEvent(this, event);
}

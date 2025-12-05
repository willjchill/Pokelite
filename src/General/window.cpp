#include "window.h"
#include "../Overworld/Overworld.h"
#include "../Overworld/Player_OW.h"
#include "../Battle/GUI_BT.h"
#include "../Battle/Battle_logic/PokemonData.h"
#include "uart_comm.h"
#include <QDebug>
#include <QApplication>
#include <QShowEvent>
#include <QRandomGenerator>
#include <QFont>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QSet>

Window::Window(QWidget *parent)
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
    // Install event filter to intercept arrow keys before QGraphicsView handles them
    view->installEventFilter(this);

    setCentralWidget(view);
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    view->setFocus();

    // Initialize game systems
    overworld = new Overworld(scene, view);
    battleSequence = new BattleSequence(scene, view);

    // Connect overworld signals
    connect(overworld, &Overworld::wildEncounterTriggered, this, &Window::onWildEncounterTriggered);
    connect(overworld, &Overworld::pvpBattleRequested, this, &Window::onPvpBattleRequested);
    
    // Connect battle sequence signals
    connect(battleSequence, &BattleSequence::battleEnded, this, &Window::onBattleEnded);

    // Initialize UART communication
    uartComm = new UartComm(this);
    connect(uartComm, &UartComm::packetReceived, this, &Window::onUartPacketReceived);
    connect(uartComm, &UartComm::playerFound, this, &Window::onPlayerFound);
    
    // Try to initialize UART (may fail if not on BeagleBone, that's okay)
    uartComm->initialize();

    // Initialize player
    initializePlayer();

    // Load default map
    overworld->loadMap("route6");
    
    // Initialize gamepad thread (for use after intro screen)
    gamepadThread = new Gamepad("/dev/input/event1", this);
    connect(gamepadThread, &Gamepad::inputReceived, this, &Window::handleGamepadInput);
    gamepadThread->start();
    
    // Initialize D-pad repeat timer for continuous movement
    dpadRepeatTimer = new QTimer(this);
    dpadRepeatTimer->setInterval(50); // ~20 times per second for smooth movement
    connect(dpadRepeatTimer, &QTimer::timeout, this, [this]() {
        if (currentDpadKey != Qt::Key_unknown && !inBattle && !findingPlayer && !overworld->isInMenu()) {
            QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, currentDpadKey, Qt::NoModifier);
            QApplication::postEvent(this, event);
        }
    });
    
    // Initialize keyboard movement timer for continuous movement (same as D-pad)
    keyboardMovementTimer = new QTimer(this);
    keyboardMovementTimer->setInterval(50); // ~20 times per second for smooth movement
    connect(keyboardMovementTimer, &QTimer::timeout, this, &Window::processKeyboardMovement);
}

Window::~Window()
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

void Window::initializePlayer()
{
    // Initialize Pokemon data if not already done
    initializePokemonDataFromJSON();
    
    // Create player if not exists
    if (!gamePlayer) {
        gamePlayer = new Player("Player", PlayerType::HUMAN);
        // Randomly select a starter Pokemon (Bulbasaur=1, Charmander=4, Squirtle=7)
        int starterDexNumbers[] = {1, 4, 7}; // Bulbasaur, Charmander, Squirtle
        int randomIndex = QRandomGenerator::global()->bounded(3);
        int starterDex = starterDexNumbers[randomIndex];
        Pokemon starter(starterDex, 5);
        gamePlayer->addPokemon(starter);
    }
    
    // Set player in overworld
    overworld->setPlayer(gamePlayer);
}

void Window::onWildEncounterTriggered()
{
    if (inBattle) return; // Already in battle
    
    startWildEncounter();
}

void Window::startWildEncounter()
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
    
    // Remove overworld zoom before battle
    if (overworld && overworld->getCamera()) {
        overworld->getCamera()->removeZoom();
    }
    
    // Start battle sequence
    battleSequence->startBattle(gamePlayer, enemyPlayer, battleSystem);
    
    inBattle = true;
}

void Window::onBattleEnded()
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
    QGraphicsScene* overworldScene = overworld->getScene();
    if (overworldScene) {
        view->setScene(overworldScene);
    }
    view->setFixedSize(480, 272);
    
    // Re-apply overworld zoom
    if (overworld && overworld->getCamera()) {
        overworld->getCamera()->setupZoom();
    }
    
    // Re-center camera on player
    Player_OW* playerItem = overworld->getPlayerItem();
    if (playerItem) {
        view->centerOn(playerItem);
    }
    
    // Clean up battle scene
    QGraphicsScene* bs = battleSequence->getScene();
    if (bs && bs != overworldScene) {
        delete bs;
    }
    
    setFocus();
    view->setFocus();
}

void Window::keyPressEvent(QKeyEvent *event)
{
    // Handle Escape/B to cancel finding player
    if (findingPlayer && (event->key() == Qt::Key_Escape || event->key() == Qt::Key_B)) {
        findingPlayer = false;
        uartComm->stopFindingPlayer();
        hideFindingPlayerPopup();
        return;
    }
    
    // Block all input while finding player (including movement)
    if (findingPlayer) {
        return; // Block all input while finding player
    }
    
    // Handle Q/SELECT key for PvP battle (dev key)
    if ((event->key() == Qt::Key_Q || event->key() == Qt::Key_Select) && !inBattle && !findingPlayer) {
        onPvpBattleRequested();
        return;
    }
    
    // Handle gamepad input simulation
    if (event->key() == Qt::Key_M && !inBattle && !findingPlayer) {
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
    
    // Handle overworld movement (only WASD, ignore arrow keys to prevent camera movement)
    // Use timer-based movement for consistent speed across platforms
    if (event->key() == Qt::Key_W || event->key() == Qt::Key_A || 
        event->key() == Qt::Key_S || event->key() == Qt::Key_D) {
        // Ignore auto-repeat events - we handle continuous movement via timer
        if (!event->isAutoRepeat()) {
            startKeyboardMovement(static_cast<Qt::Key>(event->key()));
        }
        // Check for lab entrance after movement
        checkLabEntrance();
    }
    // Explicitly accept arrow keys to prevent QGraphicsView from scrolling the camera
    else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down || 
             event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        event->accept(); // Accept to prevent propagation to QGraphicsView's default handler
    }
}

void Window::keyReleaseEvent(QKeyEvent *event)
{
    // Ignore auto-repeat releases so walking animation keeps running
    if (event->isAutoRepeat())
        return;

    // Handle keyboard movement key releases
    if (event->key() == Qt::Key_W || event->key() == Qt::Key_A || 
        event->key() == Qt::Key_S || event->key() == Qt::Key_D) {
        stopKeyboardMovement(static_cast<Qt::Key>(event->key()));
    }

    if (!inBattle) {
        overworld->handleKeyRelease(event);
    }
}

void Window::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // Ensure focus when window is shown
    QTimer::singleShot(0, this, [this]() {
        setFocus();
        view->setFocus();
        activateWindow();
    });
}

bool Window::eventFilter(QObject *obj, QEvent *event)
{
    // Intercept arrow key events on the view to prevent QGraphicsView from scrolling
    if (obj == view && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down || 
            keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right) {
            // Block arrow keys from reaching QGraphicsView's default handler
            // Forward them to Window::keyPressEvent instead so menus can handle them
            if (!inBattle) {
                // Post the event to Window so menu handlers can process it
                QKeyEvent *newEvent = new QKeyEvent(QEvent::KeyPress, keyEvent->key(), 
                                                    keyEvent->modifiers(), keyEvent->text(),
                                                    keyEvent->isAutoRepeat(), keyEvent->count());
                QApplication::postEvent(this, newEvent);
                return true; // Filter out the event (don't let QGraphicsView handle it)
            }
            // In battle, let arrow keys pass through to be handled by battle system
        }
    }
    // Let other events pass through normally
    return QMainWindow::eventFilter(obj, event);
}

void Window::handleGamepadInput(int type, int code, int value)
{
    // Linux input event types
    // EV_KEY = 1 (button events)
    // EV_ABS = 3 (analog stick/dpad events)
    
    // While finding player, allow movement (D-pad/analog stick) and B to cancel
    if (findingPlayer) {
        if (type == 1 && code == 305 && value == 1) { // B button pressed
            simulateKeyPress(Qt::Key_Escape);
            return;
        }
        // Allow D-pad and analog stick for movement - continue processing below
        if (type != 3) {
            // Block all other buttons while finding player
            return;
        }
        // Fall through to process D-pad/analog stick for movement
    }
    
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
        } else if (code == 314) { // SELECT button
            if (pressed && !inBattle && !findingPlayer) {
                onPvpBattleRequested();
            }
        }
    } else if (type == 3) { // EV_ABS - analog stick/dpad
        // ABS_X = 0 (left stick X or dpad left/right)
        // ABS_Y = 1 (left stick Y or dpad up/down)
        // ABS_HAT0X = 16 (dpad X)
        // ABS_HAT0Y = 17 (dpad Y)
        
        // Prioritize D-pad over analog stick
        if (code == 16) { // D-pad X
            // Stop analog stick if D-pad is being used
            if (value != 0) {
                // D-pad is pressed - stop analog stick
                simulateKeyRelease(Qt::Key_A);
                simulateKeyRelease(Qt::Key_D);
            }
            
            if (value == -1) { // Left
                stopDpadRepeat();
                startDpadRepeat(Qt::Key_A);
            } else if (value == 1) { // Right
                stopDpadRepeat();
                startDpadRepeat(Qt::Key_D);
            } else { // Released (value == 0)
                stopDpadRepeat();
            }
        } else if (code == 17) { // D-pad Y
            // Stop analog stick if D-pad is being used
            if (value != 0) {
                // D-pad is pressed - stop analog stick
                simulateKeyRelease(Qt::Key_W);
                simulateKeyRelease(Qt::Key_S);
            }
            
            if (value == -1) { // Up
                stopDpadRepeat();
                startDpadRepeat(Qt::Key_W);
            } else if (value == 1) { // Down
                stopDpadRepeat();
                startDpadRepeat(Qt::Key_S);
            } else { // Released (value == 0)
                stopDpadRepeat();
            }
        } else if (code == 0) { // Left stick X - only use if D-pad not active
            // Only process analog stick if D-pad is not currently being used
            if (currentDpadKey == Qt::Key_unknown || (currentDpadKey != Qt::Key_A && currentDpadKey != Qt::Key_D)) {
                if (value < -10000) { // Left threshold
                    simulateKeyPress(Qt::Key_A);
                } else if (value > 10000) { // Right threshold
                    simulateKeyPress(Qt::Key_D);
                } else { // Center
                    simulateKeyRelease(Qt::Key_A);
                    simulateKeyRelease(Qt::Key_D);
                }
            }
        } else if (code == 1) { // Left stick Y - only use if D-pad not active
            // Only process analog stick if D-pad is not currently being used
            if (currentDpadKey == Qt::Key_unknown || (currentDpadKey != Qt::Key_W && currentDpadKey != Qt::Key_S)) {
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
}

void Window::simulateKeyPress(Qt::Key key)
{
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier);
    QApplication::postEvent(this, event);
}

void Window::simulateKeyRelease(Qt::Key key)
{
    QKeyEvent *event = new QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier);
    QApplication::postEvent(this, event);
}

void Window::startDpadRepeat(Qt::Key key)
{
    // Stop any existing repeat
    stopDpadRepeat();
    
    // Set the current key and send initial press
    currentDpadKey = key;
    simulateKeyPress(key);
    
    // Start timer to repeat the key press
    dpadRepeatTimer->start();
}

void Window::stopDpadRepeat()
{
    if (dpadRepeatTimer) {
        dpadRepeatTimer->stop();
    }
    
    // Send key release for the current key
    if (currentDpadKey != Qt::Key_unknown) {
        simulateKeyRelease(currentDpadKey);
        currentDpadKey = Qt::Key_unknown;
    }
}

void Window::startKeyboardMovement(Qt::Key key)
{
    // Add key to pressed keys set
    pressedMovementKeys.insert(key);
    
    // Send initial movement event
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier);
    overworld->handleMovementInput(event);
    
    // Start timer if not already running
    if (keyboardMovementTimer && !keyboardMovementTimer->isActive()) {
        keyboardMovementTimer->start();
    }
}

void Window::stopKeyboardMovement(Qt::Key key)
{
    // Remove key from pressed keys set
    pressedMovementKeys.remove(key);
    
    // Stop timer if no keys are pressed
    if (keyboardMovementTimer && pressedMovementKeys.isEmpty()) {
        keyboardMovementTimer->stop();
        // Only send key release event when ALL movement keys are released
        // This ensures animation continues during diagonal movement
        QKeyEvent *event = new QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier);
        overworld->handleKeyRelease(event);
    }
}

void Window::processKeyboardMovement()
{
    // Only process if not in battle, not finding player, and not in menu
    if (inBattle || findingPlayer || !overworld || overworld->isInMenu()) {
        return;
    }
    
    // Process each pressed movement key
    for (Qt::Key key : pressedMovementKeys) {
        QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier);
        overworld->handleMovementInput(event);
    }
}

void Window::onPvpBattleRequested()
{
    if (inBattle || findingPlayer || !uartComm) return;
    
    findingPlayer = true;
    isBattleInitiator = true;  // We are initiating the battle
    showFindingPlayerPopup();
    uartComm->startFindingPlayer();
    
    // Hide overworld menu AFTER showing the popup
    if (overworld->isInMenu()) {
        overworld->hideOverworldMenu();
    }
}

void Window::onUartPacketReceived(const BattlePacket& packet)
{
    qDebug() << "Received UART packet type:" << static_cast<int>(packet.type) << "data:" << packet.data;
    
    switch (packet.type) {
        case PacketType::READY_BATTLE:
        {
            // Parse opponent Pokemon data from READY_BATTLE:[dex],[level]
            // This will be used when we actually start the PvP battle.
            if (!packet.data.isEmpty()) {
                const QStringList parts = packet.data.split(',');
                if (parts.size() >= 2) {
                    bool okDex = false;
                    bool okLvl = false;
                    int dex = parts[0].toInt(&okDex);
                    int lvl = parts[1].toInt(&okLvl);
                    if (okDex && okLvl && dex > 0 && lvl > 0) {
                        pvpOpponentDexNumber = dex;
                        pvpOpponentLevel = lvl;
                        qDebug() << "Stored PvP opponent Pokemon: dex" << dex << "level" << lvl;
                    }
                }
            }
            // Connection handshake / battle start is still driven by onPlayerFound signal.
            break;
        }
        case PacketType::TURN:
            // Notify battle sequence that opponent has completed their turn
            // packet.data format: "moveIndex,damage" (precalculated damage)
            if (battleSequence && inBattle) {
                int moveIndex = -1;
                int damage = -1;
                if (!packet.data.isEmpty()) {
                    const QStringList parts = packet.data.split(',');
                    if (parts.size() >= 1) {
                        bool okIdx = false;
                        moveIndex = parts[0].toInt(&okIdx);
                        if (!okIdx) moveIndex = -1;
                    }
                    if (parts.size() >= 2) {
                        bool okDmg = false;
                        damage = parts[1].toInt(&okDmg);
                        if (!okDmg) damage = -1;
                    }
                }
                battleSequence->onOpponentTurnComplete(moveIndex, damage);
            }
            break;
        case PacketType::ITEM:
            // Opponent used an item: data format "itemIndex,healAmount"
            if (battleSequence && inBattle) {
                int itemIndex = -1;
                int healAmount = 0;
                if (!packet.data.isEmpty()) {
                    const QStringList parts = packet.data.split(',');
                    if (parts.size() >= 2) {
                        bool okIdx = false;
                        bool okHeal = false;
                        itemIndex = parts[0].toInt(&okIdx);
                        healAmount = parts[1].toInt(&okHeal);
                        if (!okIdx) itemIndex = -1;
                        if (!okHeal) healAmount = 0;
                    }
                }
                // A healAmount of 0 is still valid (e.g., failed item), so always forward.
                battleSequence->onOpponentItemUsed(itemIndex, healAmount);
            }
            break;
        case PacketType::SWITCH:
            // Opponent switched Pokemon: data format "dexNumber,level,currentHP"
            if (battleSequence && inBattle) {
                int dexNumber = -1;
                int level = -1;
                int currentHP = -1;
                if (!packet.data.isEmpty()) {
                    const QStringList parts = packet.data.split(',');
                    if (parts.size() >= 2) {
                        bool okDex = false;
                        bool okLevel = false;
                        dexNumber = parts[0].toInt(&okDex);
                        level = parts[1].toInt(&okLevel);
                        if (!okDex) dexNumber = -1;
                        if (!okLevel) level = -1;
                        // Parse health if available (for backward compatibility, make it optional)
                        if (parts.size() >= 3) {
                            bool okHP = false;
                            currentHP = parts[2].toInt(&okHP);
                            if (!okHP) currentHP = -1;
                        }
                    }
                }
                battleSequence->onOpponentSwitched(dexNumber, level, currentHP);
            }
            break;
        case PacketType::TURN_ORDER:
            // Initiator sent turn order: data format "1" or "2" (1=initiator goes first, 2=responder goes first)
            if (battleSequence && inBattle) {
                int firstPlayer = 1;  // Default to initiator
                if (!packet.data.isEmpty()) {
                    bool ok = false;
                    firstPlayer = packet.data.toInt(&ok);
                    if (!ok) firstPlayer = 1;
                }
                // firstPlayer: 1 = initiator goes first, 2 = responder goes first
                // isBattleInitiator: true = we are initiator, false = we are responder
                bool weGoFirst = (firstPlayer == 1 && isBattleInitiator) || (firstPlayer == 2 && !isBattleInitiator);
                battleSequence->setInitialTurnOrder(weGoFirst);
            }
            break;
        case PacketType::LOSE:
            // Opponent has no usable Pokemon left - they lost
            if (battleSequence && inBattle) {
                battleSequence->onOpponentLost();
            }
            break;
        case PacketType::BATTLE_END:
            // Handle battle end from opponent
            break;
        default:
            break;
    }
}

void Window::onPlayerFound()
{
    if (!findingPlayer) return;
    
    hideFindingPlayerPopup();
    findingPlayer = false;
    uartComm->stopFindingPlayer();
    
    // Send READY_BATTLE packet back including our active Pokemon data so the
    // opponent can construct an identical battle state.
    int dexNumber = -1;
    int level = -1;
    if (gamePlayer && gamePlayer->getActivePokemon()) {
        const Pokemon *active = gamePlayer->getActivePokemon();
        dexNumber = active->getDexNumber();
        level = active->getLevel();
    }
    QString dataStr;
    if (dexNumber > 0 && level > 0) {
        dataStr = QString::number(dexNumber) + "," + QString::number(level);
    }
    BattlePacket readyPacket(PacketType::READY_BATTLE, dataStr);
    uartComm->sendPacket(readyPacket);
    
    // Start PvP battle
    startPvpBattle();
}

void Window::showFindingPlayerPopup()
{
    if (!overworld || !view) return;
    
    QGraphicsScene *overworldScene = overworld->getScene();
    if (!overworldScene) return;
    
    QFont font("Pokemon Fire Red", 9, QFont::Bold);
    
    // Get view center in scene coordinates (accounting for zoom) - same as menu
    QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();
    qreal centerX = viewRect.center().x();
    qreal centerY = viewRect.center().y();
    
    const qreal boxW = 180, boxH = 50;
    qreal boxX = centerX - boxW / 2;
    qreal boxY = centerY - boxH / 2;
    
    findingPlayerRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    findingPlayerRect->setBrush(QColor(240, 240, 240));
    findingPlayerRect->setPen(QPen(Qt::black, 3));
    findingPlayerRect->setZValue(20);
    overworldScene->addItem(findingPlayerRect);
    
    // Create text
    findingPlayerText = new QGraphicsTextItem("Finding player...\nPress ESC to cancel");
    findingPlayerText->setFont(font);
    findingPlayerText->setDefaultTextColor(Qt::black);
    findingPlayerText->setPos(boxX + 15, boxY + 12);
    findingPlayerText->setZValue(21);
    overworldScene->addItem(findingPlayerText);
}

void Window::hideFindingPlayerPopup()
{
    QGraphicsScene *overworldScene = overworld ? overworld->getScene() : nullptr;
    
    if (findingPlayerRect) {
        if (overworldScene) {
            overworldScene->removeItem(findingPlayerRect);
        }
        delete findingPlayerRect;
        findingPlayerRect = nullptr;
    }
    
    if (findingPlayerText) {
        if (overworldScene) {
            overworldScene->removeItem(findingPlayerText);
        }
        delete findingPlayerText;
        findingPlayerText = nullptr;
    }
}

void Window::startPvpBattle()
{
    if (inBattle) return;
    
    // Clean up previous enemy player if exists
    if (enemyPlayer) {
        delete enemyPlayer;
    }
    
    // Create enemy player using Pokemon data received over UART.
    // Fallback to a simple default if, for some reason, we did not receive data.
    enemyPlayer = new Player("Opponent", PlayerType::HUMAN);
    int dexNumber = pvpOpponentDexNumber;
    int level = pvpOpponentLevel;
    if (dexNumber <= 0 || level <= 0) {
        dexNumber = 1;
        level = 5;
    }
    Pokemon enemyPokemon(dexNumber, level);
    enemyPlayer->addPokemon(enemyPokemon);
    
    // Auto-heal all Pokemon in the team before battle (assume both players start at full health)
    if (gamePlayer) {
        auto& team = gamePlayer->getTeam();
        for (auto& pokemon : team) {
            pokemon.heal(pokemon.getMaxHP());
        }
    }
    
    // Initialize battle system with PvP mode (isWild = false)
    if (battleSystem) {
        delete battleSystem;
    }
    battleSystem = new BattleSystem();
    battleSystem->initializeBattle(gamePlayer, enemyPlayer, false);
    battleSystem->setPvpMode(true);  // Enable PvP mode
    
    // Remove overworld zoom before battle
    if (overworld && overworld->getCamera()) {
        overworld->getCamera()->removeZoom();
    }
    
    // Start battle sequence
    battleSequence->startBattle(gamePlayer, enemyPlayer, battleSystem);
    battleSequence->setUartComm(uartComm);  // Pass UART to battle sequence
    
    inBattle = true;
    
    // If we are the initiator, determine turn order and send TURN_ORDER packet
    if (isBattleInitiator && gamePlayer && enemyPlayer && gamePlayer->getActivePokemon() && enemyPlayer->getActivePokemon()) {
        Pokemon* playerPoke = gamePlayer->getActivePokemon();
        Pokemon* enemyPoke = enemyPlayer->getActivePokemon();
        
        int playerSpeed = playerPoke->getStats().speed;
        int enemySpeed = enemyPoke->getStats().speed;
        
        int firstPlayer = 1;  // Default to initiator (us)
        
        if (enemySpeed > playerSpeed) {
            // Enemy (responder) goes first
            firstPlayer = 2;
        } else if (enemySpeed == playerSpeed) {
            // Speed tie: 50/50 random chance
            firstPlayer = (QRandomGenerator::global()->bounded(2) == 0) ? 1 : 2;
        }
        // else: playerSpeed > enemySpeed, firstPlayer stays 1 (initiator goes first)
        
        // Send TURN_ORDER packet
        QString dataStr = QString::number(firstPlayer);
        BattlePacket turnOrderPacket(PacketType::TURN_ORDER, dataStr);
        uartComm->sendPacket(turnOrderPacket);
        
        // Set our turn order based on who goes first
        bool weGoFirst = (firstPlayer == 1);
        battleSequence->setInitialTurnOrder(weGoFirst);
    }
    // If we're not the initiator, we'll wait for TURN_ORDER packet (handled in onUartPacketReceived)
}

void Window::setPlayerSpawnPosition(const QPointF &pos)
{
    if (overworld) {
        overworld->setPlayerPosition(pos);

        // Set player to face front when exiting lab
        if (overworld->getPlayerItem()) {
            overworld->getPlayerItem()->setDirection("front");
            overworld->getPlayerItem()->stopAnimation();
        }
    }
}

void Window::checkLabEntrance()
{
    if (!overworld || !overworld->getPlayerItem()) return;

    QPointF playerPos = overworld->getPlayerItem()->getPosition();

    // Check if player is at lab entrance position (303, 199) with some tolerance
    if (qAbs(playerPos.x() - 303) < 10 && qAbs(playerPos.y() - 199) < 10) {
        emit returnToLab();
    }
}

void Window::clearMovementState()
{
    // Clear all pressed movement keys
    pressedMovementKeys.clear();

    // Stop keyboard movement timer
    if (keyboardMovementTimer && keyboardMovementTimer->isActive()) {
        keyboardMovementTimer->stop();
    }

    // Stop D-pad repeat
    stopDpadRepeat();

    // Stop player animation in overworld
    if (overworld && overworld->getPlayerItem()) {
        overworld->getPlayerItem()->stopAnimation();
    }
}

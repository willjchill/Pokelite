// battle_system.cpp
// BATTLE LOGIC + UI SETUP (NO animations)

#include "mainwindow.h"
#include <QDebug>
#include <QBrush>
#include <QPen>
#include <QTimer>
#include <QRandomGenerator>

// ======================================================
// ================ BATTLE STATE (LOCAL) ================
// ======================================================

// Lives only in this file. Holds HP and move info.
struct BattleState {
    int playerHP    = 45;
    int playerMaxHP = 45;

    int enemyHP     = 39;
    int enemyMaxHP  = 39;

    // Dummy move names as requested
    QString playerMoves[3] = {
        "ATTACK 1",
        "ATTACK 2",
        "ATTACK 3"
    };

    QString enemyMoves[2] = {
        "EMBER",
        "SCRATCH"
    };

    bool waitingForPlayerMove = false; // true when in MOVE menu
    bool waitingForEnemyTurn  = false; // enemy is about to attack / lock input
};

// Single instance for the battle
static BattleState battleState;

// Separate MOVE MENU UI (white box + text)
static QGraphicsRectItem *moveMenuRect = nullptr;
static QVector<QGraphicsTextItem*> moveMenuOptions;

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

// ======================================================
// ===================== BATTLE START ===================
// ======================================================

void MainWindow::tryWildEncounter()
{
    // Remember overworld scene so we can return later
    overworldScene = scene;

    // Reset state
    inBattle        = true;
    inBattleMenu    = true;
    battleMenuIndex = 0;
    battleState     = BattleState();   // reset HP, flags, moves
    fullBattleText.clear();
    battleTextIndex = 0;

    // Clean any leftover move menu from previous battle
    destroyMoveMenu(battleScene);

    // New battle scene
    battleScene = new QGraphicsScene(0, 0, 480, 272, this);

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
        battleTrainerItem->setZValue(1);
    }

    // Enemy Pokémon sprite
    QPixmap enemyPx(":/assets/battle/charizard.png");
    if (!enemyPx.isNull()) {
        float sx = 140.0f / enemyPx.width();
        float sy = 140.0f / enemyPx.height();
        float scale = std::min(sx, sy);

        battleEnemyItem = battleScene->addPixmap(enemyPx);
        battleEnemyItem->setScale(scale);

        qreal w = enemyPx.width() * scale;
        battleEnemyItem->setPos(480 - w - 30, 30);
        battleEnemyItem->setZValue(1);
    }

    // HP bars, dialogue box, command box, etc.
    setupBattleUI();

    // Switch to battle scene
    view->setScene(battleScene);
    view->setFixedSize(480, 272);
    setFixedSize(480, 272);

    setFocus();
    view->setFocus();

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

    fullBattleText   = "What will BULBASAUR do?";
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

    // HP bars start full
    enemyHpFill->setRect(0, 0, 62, 6);
    playerHpFill->setRect(0, 0, 86, 6);
}

// ======================================================
// ================== INPUT HANDLING ====================
// ======================================================

void MainWindow::handleBattleKey(QKeyEvent *event)
{
    if (!inBattle)
        return;

    // Ignore input during enemy turn / fade
    if (battleState.waitingForEnemyTurn)
        return;

    bool inMoveMenu = battleState.waitingForPlayerMove;

    int maxIndex = inMoveMenu ? moveMenuOptions.size()
                              : battleMenuOptions.size();
    if (maxIndex == 0)
        return;

    int key = event->key();

    // ---- CURSOR MOVEMENT (arrows + WASD) ----
    if (key == Qt::Key_Left || key == Qt::Key_A) {
        if (battleMenuIndex % 2 == 1)
            battleMenuIndex--;
    }
    else if (key == Qt::Key_Right || key == Qt::Key_D) {
        if (battleMenuIndex % 2 == 0 && battleMenuIndex + 1 < maxIndex)
            battleMenuIndex++;
    }
    else if (key == Qt::Key_Up || key == Qt::Key_W) {
        if (battleMenuIndex >= 2)
            battleMenuIndex -= 2;
    }
    else if (key == Qt::Key_Down || key == Qt::Key_S) {
        if (battleMenuIndex + 2 < maxIndex)
            battleMenuIndex += 2;
    }
    else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        // Confirm selection
        if (inMoveMenu)
            playerChoseMove(battleMenuIndex);
        else if (inBattleMenu)
            playerSelectedOption(battleMenuIndex);
        return;
    }

    // ---- UPDATE CURSOR ----
    if (inMoveMenu && !moveMenuOptions.isEmpty()) {
        QGraphicsTextItem *target = moveMenuOptions[battleMenuIndex];
        battleCursorSprite->setPos(target->pos().x() - 28,
                                   target->pos().y() - 2);
    } else if (!inMoveMenu && !battleMenuOptions.isEmpty()) {
        QGraphicsTextItem *target = battleMenuOptions[battleMenuIndex];
        battleCursorSprite->setPos(target->pos().x() - 28,
                                   target->pos().y() - 2);
    }
}

// ======================================================
// =============== CURSOR / COLOR HELPERS ===============
// ======================================================

void MainWindow::updateBattleCursor()
{
    if (!battleCursorSprite) return;

    if (battleState.waitingForPlayerMove && !moveMenuOptions.isEmpty()) {
        QGraphicsTextItem *target = moveMenuOptions[battleMenuIndex];
        battleCursorSprite->setPos(target->pos().x() - 28,
                                   target->pos().y() - 2);
    } else if (!battleMenuOptions.isEmpty()) {
        QGraphicsTextItem *target = battleMenuOptions[battleMenuIndex];
        battleCursorSprite->setPos(target->pos().x() - 28,
                                   target->pos().y() - 2);
    }
}

void MainWindow::setHpColor(QGraphicsRectItem *hpBar, float hpPercent)
{
    if (hpPercent > 0.5f)       hpBar->setBrush(QColor("#3CD75F"));
    else if (hpPercent > 0.2f)  hpBar->setBrush(QColor("#FFCB43"));
    else                        hpBar->setBrush(QColor("#FF4949"));
}

// ======================================================
// ============ BATTLE FLOW: COMMAND MENU ===============
// ======================================================

void MainWindow::playerSelectedOption(int index)
{
    // 0 = FIGHT
    if (index == 0) {
        // ===== BUILD MOVE MENU =====
        destroyMoveMenu(battleScene);

        QString labels[4] = {
            battleState.playerMoves[0],
            battleState.playerMoves[1],
            battleState.playerMoves[2],
            "BACK"
        };

        QFont f("Pokemon Fire Red", 10, QFont::Bold);

        const qreal boxW = 230, boxH = 60;
        qreal boxX = dialogueBoxSprite->pos().x() + 10;
        qreal boxY = dialogueBoxSprite->pos().y() - boxH - 4;

        moveMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
        moveMenuRect->setBrush(Qt::white);
        moveMenuRect->setPen(QPen(Qt::black, 2));
        moveMenuRect->setZValue(3);
        battleScene->addItem(moveMenuRect);

        moveMenuOptions.clear();
        for (int i = 0; i < 4; ++i) {
            QGraphicsTextItem *t = new QGraphicsTextItem(labels[i]);
            t->setFont(f);
            t->setDefaultTextColor(Qt::black);

            int row = i / 2;
            int col = i % 2;
            t->setPos(boxX + 16 + col * 100,
                      boxY + 14 + row * 20);
            t->setZValue(4);

            battleScene->addItem(t);
            moveMenuOptions.push_back(t);
        }

        // Enter MOVE MENU mode
        battleState.waitingForPlayerMove = true;
        inBattleMenu                     = false;
        battleMenuIndex                  = 0;

        updateBattleCursor();
        return;
    }

    // 1 = BAG (placeholder text)
    if (index == 1) {
        fullBattleText  = "Your BAG is empty!";
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);
        return;
    }

    // 2 = POKEMON (placeholder text)
    if (index == 2) {
        fullBattleText  = "You have no other POKEMON!";
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);
        return;
    }

    // 3 = RUN
    if (index == 3) {
        fullBattleText  = "You got away safely!";
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);

        inBattleMenu = false;
        battleState.waitingForEnemyTurn = true; // lock input

        QTimer::singleShot(600, [=]() {
            fadeOutBattleScreen([=]() {
                battleState.waitingForEnemyTurn = false;
                closeBattleReturnToMap();
            });
        });
        return;
    }
}

// ======================================================
// ============ BATTLE FLOW: MOVE MENU ==================
// ======================================================

void MainWindow::playerChoseMove(int moveIndex)
{
    // 3 = BACK
    if (moveIndex == 3) {
        destroyMoveMenu(battleScene);

        battleState.waitingForPlayerMove = false;
        inBattleMenu                     = true;
        battleMenuIndex                  = 0;

        fullBattleText  = "What will BULBASAUR do?";
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);

        updateBattleCursor();
        return;
    }

    // Real move chosen (dummy text)
    QString moveName = battleState.playerMoves[moveIndex];
    fullBattleText  = "BULBASAUR used " + moveName + "!";
    battleTextIndex = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(22);

    destroyMoveMenu(battleScene);
    battleState.waitingForPlayerMove = false;
    inBattleMenu                     = false;
    battleState.waitingForEnemyTurn  = true;  // lock input until enemy turn runs

    // ==========================
    // PLACEHOLDER DAMAGE LOGIC
    // ==========================
    int damage = 8;
    battleState.enemyHP -= damage;
    if (battleState.enemyHP < 0) battleState.enemyHP = 0;

    float hpPercent = (float)battleState.enemyHP / (float)battleState.enemyMaxHP;
    enemyHpFill->setRect(0, 0, 62 * hpPercent, 6);
    setHpColor(enemyHpFill, hpPercent);

    // Enemy fainted?
    if (battleState.enemyHP == 0) {
        fullBattleText  = "The foe fainted!";
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

    // Enemy turn
    QTimer::singleShot(1000, [=]() {
        enemyTurn();
    });
}

// ======================================================
// ============== BATTLE FLOW: ENEMY TURN ===============
// ======================================================

void MainWindow::enemyTurn()
{
    battleState.waitingForEnemyTurn = false;

    // Pick random enemy move
    int mvIndex = QRandomGenerator::global()->bounded(2);
    QString moveName = battleState.enemyMoves[mvIndex];

    fullBattleText  = "The foe used " + moveName + "!";
    battleTextIndex = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(22);

    // ==========================
    // PLACEHOLDER DAMAGE LOGIC
    // ==========================
    int damage = 7;
    battleState.playerHP -= damage;
    if (battleState.playerHP < 0) battleState.playerHP = 0;

    float hpPercent = (float)battleState.playerHP / (float)battleState.playerMaxHP;
    playerHpFill->setRect(0, 0, 86 * hpPercent, 6);
    setHpColor(playerHpFill, hpPercent);

    // Player fainted?
    if (battleState.playerHP == 0) {
        fullBattleText  = "BULBASAUR fainted...";
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
        fullBattleText  = "What will BULBASAUR do?";
        battleTextIndex = 0;
        battleTextItem->setPlainText("");
        battleTextTimer.start(22);

        battleState.waitingForPlayerMove = false;
        inBattleMenu                     = true;
        battleMenuIndex                  = 0;

        destroyMoveMenu(battleScene);  // just in case
        updateBattleCursor();
    });
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

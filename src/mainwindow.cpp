#include "mainwindow.h"
#include <QDebug>
#include <QBrush>
#include <QPen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Overworld window size
    setFixedSize(380, 639);

    // Scene
    scene = new QGraphicsScene(0, 0, 380, 639, this);

    // View
    view = new QGraphicsView(scene, this);
    view->setFixedSize(380, 639);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // IMPORTANT: show the view as the central widget
    setCentralWidget(view);

    // Background map
    background = scene->addPixmap(QPixmap(":/assets/background.png"));
    background->setZValue(0);

    // Load collision mask
    collisionMask.load(":/assets/collision.png");
    if (collisionMask.isNull())
        qDebug() << "ERROR: collision.png failed to load!";

    // Load tall grass mask
    tallGrassMask.load(":/assets/tallgrass.png");
    if (tallGrassMask.isNull())
        qDebug() << "ERROR: tallgrass.png failed to load!";

    // Load walking animations
    loadAnimations();

    // Player setup
    currentDirection = "front";
    frameIndex = 1;
    isMoving = false;

    player = scene->addPixmap(animations[currentDirection][frameIndex]);
    player->setZValue(1);
    player->setPos(160, 300);

    // Walk animation timer
    connect(&animationTimer, &QTimer::timeout, this, [this]() {
        auto &frames = animations[currentDirection];
        frameIndex = (frameIndex + 1) % frames.size();
        player->setPixmap(frames[frameIndex]);
    });

    // Fade timer for battle
    connect(&battleFadeTimer, &QTimer::timeout, this, [this]() {
        if (!battleFadeRect) {
            battleFadeTimer.stop();
            return;
        }
        qreal op = battleFadeRect->opacity();
        op -= 0.1;
        if (op <= 0.0) {
            battleFadeRect->setOpacity(0.0);
            if (battleScene) {
                battleScene->removeItem(battleFadeRect);
            }
            delete battleFadeRect;
            battleFadeRect = nullptr;
            battleFadeTimer.stop();
        } else {
            battleFadeRect->setOpacity(op);
        }
    });

    // Text typewriter timer
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

MainWindow::~MainWindow() {}

void MainWindow::loadAnimations()
{
    animations["front"] = {
        QPixmap(":/assets/front1.png"),
        QPixmap(":/assets/front2.png"),
        QPixmap(":/assets/front3.png")
    };
    animations["back"] = {
        QPixmap(":/assets/back1.png"),
        QPixmap(":/assets/back2.png"),
        QPixmap(":/assets/back3.png")
    };
    animations["left"] = {
        QPixmap(":/assets/left1.png"),
        QPixmap(":/assets/left2.png"),
        QPixmap(":/assets/left3.png")
    };
    animations["right"] = {
        QPixmap(":/assets/right1.png"),
        QPixmap(":/assets/right2.png"),
        QPixmap(":/assets/right3.png")
    };
}

// ---------------------------------------------------------
// PIXEL COLLISION HELPERS
// ---------------------------------------------------------

bool MainWindow::isSolidPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= collisionMask.width() ||
        y >= collisionMask.height())
        return true; // outside map = solid

    QColor pixel = collisionMask.pixelColor(x, y);

    // Black = solid wall
    return (pixel.red() < 30 &&
            pixel.green() < 30 &&
            pixel.blue() < 30);
}

bool MainWindow::isSlowPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= collisionMask.width() ||
        y >= collisionMask.height())
        return false;

    QColor pixel = collisionMask.pixelColor(x, y);

    // Blue = slow zone
    return (pixel.blue() > 200 &&
            pixel.red() < 80 &&
            pixel.green() < 80);
}

bool MainWindow::isGrassPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= tallGrassMask.width() ||
        y >= tallGrassMask.height())
        return false;

    QColor px = tallGrassMask.pixelColor(x, y);

    // Your tall grass is pink-ish (e.g., 255, 64, 255)
    return (px.red() > 200 &&
            px.blue() > 200 &&
            px.green() < 100);
}

// ---------------------------------------------------------
// MOVEMENT + ANIMATION
// ---------------------------------------------------------

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (inBattle) {
        handleBattleKey(event);   // Move in battle menu
        return;
    }


    float dx = 0;
    float dy = 0;

    switch (event->key())
    {
    case Qt::Key_W: dy = -1; currentDirection = "back";  break;
    case Qt::Key_S: dy = +1; currentDirection = "front"; break;
    case Qt::Key_A: dx = -1; currentDirection = "left";  break;
    case Qt::Key_D: dx = +1; currentDirection = "right"; break;
    default:
        return;
    }

    // Check pixel under feet
    QPointF oldPos = player->pos();
    QPointF tryPos(oldPos.x() + dx * speed, oldPos.y() + dy * speed);

    int px = tryPos.x() + player->boundingRect().width() / 2;
    int py = tryPos.y() + player->boundingRect().height() - 4;

    // SLOW ZONE DETECTED: reduce speed
    float finalSpeed = speed;
    if (isSlowPixel(px, py))
        finalSpeed = speed * 0.4f;

    // Recalculate tryPos with slow speed
    tryPos = QPointF(oldPos.x() + dx * finalSpeed,
                     oldPos.y() + dy * finalSpeed);

    // Check collision again
    px = tryPos.x() + player->boundingRect().width() / 2;
    py = tryPos.y() + player->boundingRect().height() - 4;

    // If solid → do not move
    if (!isSolidPixel(px, py))
        player->setPos(tryPos);

    // 100% encounter when standing in grass (if not already in battle)
    if (!inBattle && isGrassPixel(px, py)) {
        tryWildEncounter();
    }

    // Start animation
    if (!isMoving && (dx != 0 || dy != 0))
    {
        isMoving = true;
        frameIndex = 1;
        animationTimer.start(120);
    }

    clampPlayer();
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_W ||
        event->key() == Qt::Key_A ||
        event->key() == Qt::Key_S ||
        event->key() == Qt::Key_D)
    {
        isMoving = false;
        animationTimer.stop();
        frameIndex = 1;  // idle frame
        player->setPixmap(animations[currentDirection][frameIndex]);
    }
}

void MainWindow::clampPlayer()
{
    QRectF bounds = player->boundingRect();
    QPointF pos = player->pos();

    if (pos.x() < 0) pos.setX(0);
    if (pos.x() > 380 - bounds.width())
        pos.setX(380 - bounds.width());

    if (pos.y() < 0) pos.setY(0);
    if (pos.y() > 639 - bounds.height())
        pos.setY(639 - bounds.height());

    player->setPos(pos);
}

// ---------------------------------------------------------
// BATTLE LOGIC + UI
// ---------------------------------------------------------

void MainWindow::tryWildEncounter()
{
    inBattle = true;
    inBattleMenu = false;

    // 1. Create battle scene first
    battleScene = new QGraphicsScene(0, 0, 480, 272, this);

    // --- Background ---
    QPixmap battleBg(":/assets/battle/battle_bg.png");
    if (battleBg.isNull()) {
        battleBg = QPixmap(480, 272);
        battleBg.fill(Qt::black);
    }
    battleScene->addPixmap(battleBg)->setZValue(0);

    // --- Trainer ---
    QPixmap trainer(":/assets/battle/trainer.png");
    if (!trainer.isNull()) {
        float maxW = 120.0f, maxH = 120.0f;
        float sx = maxW / trainer.width();
        float sy = maxH / trainer.height();
        float scale = std::min(sx, sy);

        battleTrainerItem = battleScene->addPixmap(trainer);
        battleTrainerItem->setScale(scale);

        qreal trainerH = trainer.height() * scale;
        battleTrainerItem->setPos(20, 272 - trainerH - 60);
        battleTrainerItem->setZValue(1);
    }

    // --- Enemy ---
    QPixmap enemy(":/assets/battle/charizard.png");
    if (!enemy.isNull()) {
        float maxW = 140.0f, maxH = 140.0f;
        float sx = maxW / enemy.width();
        float sy = maxH / enemy.height();
        float scale = std::min(sx, sy);

        battleEnemyItem = battleScene->addPixmap(enemy);
        battleEnemyItem->setScale(scale);

        qreal enemyW = enemy.width() * scale;
        battleEnemyItem->setPos(480 - enemyW - 30, 30);
        battleEnemyItem->setZValue(1);
    }

    // 2. Build UI (HP boxes, textbox, command box)
    setupBattleUI();

    // 3. Switch to the battle scene FIRST
    view->setScene(battleScene);
    view->setFixedSize(480, 272);
    setFixedSize(480, 272);

    // 4. Fade-in
    battleZoomReveal();

    view->setFocus();



    // 5. Menu animation AFTER fade begins
    slideInCommandMenu();
}

void MainWindow::setupBattleUI()
{
    // Common battle font
    QFont battleFont("Pokemon Fire Red", 10, QFont::Bold);

    //
    // 1. Enemy HP window
    //
    QPixmap enemyHPBox(":/assets/battle/ui/hp_box_enemy.png");
    enemyHpBackSprite = battleScene->addPixmap(enemyHPBox);
    enemyHpBackSprite->setPos(20, 10);
    enemyHpBackSprite->setZValue(2);

    // enemy HP bar
    enemyHpMask = new QGraphicsRectItem(0, 0, 64, 8);
    enemyHpMask->setBrush(QColor("#F8F8F0"));
    enemyHpMask->setPen(Qt::NoPen);
    enemyHpMask->setPos(enemyHpBackSprite->pos().x() + 30,
                        enemyHpBackSprite->pos().y() + 18);
    enemyHpMask->setZValue(3);
    battleScene->addItem(enemyHpMask);

    enemyHpFill = new QGraphicsRectItem(0, 0, 62, 6);
    enemyHpFill->setBrush(QColor("#7BD77B"));
    enemyHpFill->setPen(Qt::NoPen);
    enemyHpFill->setPos(enemyHpBackSprite->pos().x() + 31,
                        enemyHpBackSprite->pos().y() + 19);
    enemyHpFill->setZValue(4);
    battleScene->addItem(enemyHpFill);


    //
    // 2. Player HP window
    //
    QPixmap playerHPBox(":/assets/battle/ui/hp_box_player.png");
    playerHpBackSprite = battleScene->addPixmap(playerHPBox);
    playerHpBackSprite->setPos(260, 140);
    playerHpBackSprite->setZValue(2);

    // player HP bar
    playerHpMask = new QGraphicsRectItem(0, 0, 88, 8);
    playerHpMask->setBrush(QColor("#F8F8F0"));
    playerHpMask->setPen(Qt::NoPen);
    playerHpMask->setPos(playerHpBackSprite->pos().x() + 42,
                         playerHpBackSprite->pos().y() + 29);
    playerHpMask->setZValue(3);
    battleScene->addItem(playerHpMask);

    playerHpFill = new QGraphicsRectItem(0, 0, 86, 6);
    playerHpFill->setBrush(QColor("#7BD77B"));
    playerHpFill->setPen(Qt::NoPen);
    playerHpFill->setPos(playerHpBackSprite->pos().x() + 43,
                         playerHpBackSprite->pos().y() + 28);
    playerHpFill->setZValue(4);
    battleScene->addItem(playerHpFill);


    //
    // 3. Dialogue box
    //
    QPixmap dialogBoxPx(":/assets/battle/ui/dialogue_box.png");

    if (!dialogBoxPx.isNull()) {
        dialogueBoxSprite = battleScene->addPixmap(dialogBoxPx);

        // IMPORTANT: Scale horizontally to fill entire width
        double scaleX = 480.0 / dialogBoxPx.width();
        double scaleY = 72.0 / dialogBoxPx.height();   // GBA height

        dialogueBoxSprite->setScale(std::min(scaleX, scaleY));

        // Position at the VERY bottom, full width
        dialogueBoxSprite->setPos(0, 272 - (dialogueBoxSprite->boundingRect().height() * dialogueBoxSprite->scale()));

        dialogueBoxSprite->setZValue(2);
    }

    battleTextItem = new QGraphicsTextItem();
    battleTextItem->setDefaultTextColor(Qt::white);
    battleTextItem->setFont(battleFont);

    // LEFT ALIGNED TEXT LIKE FIRE RED
    battleTextItem->setPos(
        dialogueBoxSprite->pos().x() + 18,     // left margin
        dialogueBoxSprite->pos().y() + 14      // top margin
        );

    battleTextItem->setZValue(3);
    battleScene->addItem(battleTextItem);

    // Continue with typewriter setup
    fullBattleText  = "What will BULBASAUR do?"; // MODIFY THIS SO IT FITS THE POKEMON
    battleTextIndex = 0;
    battleTextItem->setPlainText("");
    battleTextTimer.start(30);

    //
    // 4. Command box (scaled 1.3)
    //
    QPixmap commandBoxPx(":/assets/battle/ui/command_box.png");
    commandBoxSprite = battleScene->addPixmap(commandBoxPx);
    commandBoxSprite->setPos(480 - 160 - 50, 272 - 64 - 17);
    commandBoxSprite->setScale(1.3);
    commandBoxSprite->setZValue(2);


    //
    // 5. Invisible menu anchors
    //
    battleMenuOptions.clear();

    int px[4] = { 25, 100, 25, 100 };
    int py[4] = { 18, 18, 42, 42 };

    for (int i = 0; i < 4; i++) {
        QGraphicsTextItem *placeholder = new QGraphicsTextItem(QString::number(i));
        placeholder->setVisible(false);

        placeholder->setPos(
            commandBoxSprite->pos().x() + px[i] * commandBoxSprite->scale(),
            commandBoxSprite->pos().y() + py[i] * commandBoxSprite->scale()
            );

        battleScene->addItem(placeholder);
        battleMenuOptions.push_back(placeholder);
    }


    //
    // 6. Cursor (BIG + properly aligned)

    QPixmap arrowPx(":/assets/battle/ui/arrow_cursor.png");
    if (!arrowPx.isNull()) {

        battleCursorSprite = battleScene->addPixmap(arrowPx);

        // Make cursor MUCH bigger
        battleCursorSprite->setScale(2.0);
        battleCursorSprite->setZValue(5);

        battleMenuIndex = 0;

        // -------- PERFECT FRLG ALIGNMENT --------
        int cursorOffsetX = -28;   // move left of text anchor
        int cursorOffsetY = -2;    // move slightly down to align with text middle
        // ----------------------------------------

        battleCursorSprite->setPos(
            battleMenuOptions[battleMenuIndex]->pos().x() + cursorOffsetX,
            battleMenuOptions[battleMenuIndex]->pos().y() + cursorOffsetY
            );
    }
}

void MainWindow::handleBattleKey(QKeyEvent *event)
{
    // W / UP
    if (event->key() == Qt::Key_W || event->key() == Qt::Key_Up) {
        if (battleMenuIndex >= 2)
            battleMenuIndex -= 2;
        animateMenuSelection(battleMenuIndex);
    }

    // S / DOWN
    else if (event->key() == Qt::Key_S || event->key() == Qt::Key_Down) {
        if (battleMenuIndex <= 1)
            battleMenuIndex += 2;
        animateMenuSelection(battleMenuIndex);
    }

    // A / LEFT
    else if (event->key() == Qt::Key_A || event->key() == Qt::Key_Left) {
        if (battleMenuIndex % 2 == 1)
            battleMenuIndex -= 1;
        animateMenuSelection(battleMenuIndex);
    }

    // D / RIGHT
    else if (event->key() == Qt::Key_D || event->key() == Qt::Key_Right) {
        if (battleMenuIndex % 2 == 0)
            battleMenuIndex += 1;
        animateMenuSelection(battleMenuIndex);
    }

    // ENTER selects
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        qDebug() << "Selected: " << battleMenuIndex;
    }

    // UPDATE CURSOR POSITION
    int cursorOffsetX = -25;
    int cursorOffsetY = -5;

    battleCursorSprite->setPos(
        battleMenuOptions[battleMenuIndex]->pos().x() + cursorOffsetX,
        battleMenuOptions[battleMenuIndex]->pos().y() + cursorOffsetY
        );
}

void MainWindow::updateBattleCursor()
{
    if (!battleCursorSprite) return;
    if (battleMenuOptions.isEmpty()) return;

    battleCursorSprite->setPos(
        battleMenuOptions[battleMenuIndex]->pos().x() - 16,
        battleMenuOptions[battleMenuIndex]->pos().y() - 1
        );
}

void MainWindow::setHpColor(QGraphicsRectItem *hpBar, float hpPercent)
{
    if (hpPercent > 0.50f)
        hpBar->setBrush(QColor("#3CD75F")); // green
    else if (hpPercent > 0.20f)
        hpBar->setBrush(QColor("#FFCB43")); // yellow
    else
        hpBar->setBrush(QColor("#FF4949")); // red
}

void MainWindow::fadeInBattleScreen()
{
    // Fully black overlay at start
    battleFadeRect = new QGraphicsRectItem(0, 0, 480, 272);
    battleFadeRect->setBrush(Qt::black);
    battleFadeRect->setOpacity(1.0);       // Full black
    battleFadeRect->setZValue(9999);
    battleScene->addItem(battleFadeRect);

    // MUCH MORE OBVIOUS FADE (slower + smoother)
    battleFadeTimer.stop();
    connect(&battleFadeTimer, &QTimer::timeout, this, [this]() {

        if (!battleFadeRect) {
            battleFadeTimer.stop();
            return;
        }

        qreal op = battleFadeRect->opacity();

        // Ultra-obvious fade: slow + strong
        op -= 0.010;       // (0.010 per frame @60fps → ~1.6 seconds)

        if (op <= 0.0) {
            // Done fading
            battleFadeRect->setOpacity(0.0);
            battleScene->removeItem(battleFadeRect);
            delete battleFadeRect;
            battleFadeRect = nullptr;
            battleFadeTimer.stop();
        } else {
            battleFadeRect->setOpacity(op);
        }
    });

    battleFadeTimer.start(16);   // 60 FPS fade
}

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
        connect(fadeAnim, &QPropertyAnimation::finished, this, onFinished);
    }

    fadeAnim->start();
}

void MainWindow::battleZoomReveal()
{
    // Dramatic full black mask
    QGraphicsPathItem *fadeMask = new QGraphicsPathItem();
    fadeMask->setBrush(Qt::black);
    fadeMask->setPen(Qt::NoPen);
    fadeMask->setZValue(9999);
    battleScene->addItem(fadeMask);

    // Full-screen rectangle
    QPainterPath fullScreen;
    fullScreen.addRect(0, 0, 480, 272);

    // MUCH more dramatic animation
    QVariantAnimation *anim = new QVariantAnimation(this);
    anim->setDuration(2000);                 // ← 2 seconds for DRAMA
    anim->setStartValue(0);
    anim->setEndValue(1100);                 // ← bigger radius = smoother edge

    // Add easing curve for dramatic feel
    anim->setEasingCurve(QEasingCurve::OutCubic);
    // Could also use:
    // QEasingCurve::InOutQuad (gentle)
    // QEasingCurve::OutBack (bouncy overshoot)
    // QEasingCurve::OutExpo (VERY dramatic)

    connect(anim, &QVariantAnimation::valueChanged, this,
            [=](const QVariant &v)
            {
                qreal r = v.toReal();

                // Center of screen
                qreal cx = 240;
                qreal cy = 136;

                // Circle hole path
                QPainterPath hole;
                hole.addEllipse(cx - r, cy - r, r * 2, r * 2);

                // Subtract hole from white
                QPainterPath reveal = fullScreen.subtracted(hole);

                fadeMask->setPath(reveal);
            }
            );

    connect(anim, &QVariantAnimation::finished, this,
            [=]()
            {
                battleScene->removeItem(fadeMask);
                delete fadeMask;
            }
            );

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::slideInCommandMenu()
{
    // replicate the menu anchor offsets used in setupBattleUI
    int px[4] = { 25, 100, 25, 100 };
    int py[4] = { 18, 18, 42, 42 };

    int targetY = commandBoxSprite->pos().y();

    QVariantAnimation *anim = new QVariantAnimation(this);
    anim->setDuration(250);
    anim->setStartValue(300);   // below screen
    anim->setEndValue(targetY);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(anim, &QVariantAnimation::valueChanged, this,
            [=](const QVariant &value)
            {
                int newY = value.toInt();
                commandBoxSprite->setY(newY);

                // move invisible text anchors with the box
                for (int i = 0; i < 4; i++)
                {
                    battleMenuOptions[i]->setPos(
                        commandBoxSprite->pos().x() + px[i] * commandBoxSprite->scale(),
                        newY + py[i] * commandBoxSprite->scale()
                        );
                }

                // move cursor with its selected option
                battleCursorSprite->setY(
                    battleMenuOptions[battleMenuIndex]->pos().y() - 5
                    );
            });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}


void MainWindow::slideOutCommandMenu(std::function<void()> onFinished)
{
    int px[4] = { 25, 100, 25, 100 };
    int py[4] = { 18, 18, 42, 42 };

    QVariantAnimation *anim = new QVariantAnimation(this);
    anim->setDuration(200);
    anim->setStartValue(commandBoxSprite->pos().y());
    anim->setEndValue(300); // below screen
    anim->setEasingCurve(QEasingCurve::InCubic);

    connect(anim, &QVariantAnimation::valueChanged, this,
            [=](const QVariant &value)
            {
                int newY = value.toInt();
                commandBoxSprite->setY(newY);

                for (int i = 0; i < 4; i++)
                {
                    battleMenuOptions[i]->setPos(
                        commandBoxSprite->pos().x() + px[i] * commandBoxSprite->scale(),
                        newY + py[i] * commandBoxSprite->scale()
                        );
                }

                battleCursorSprite->setY(
                    battleMenuOptions[battleMenuIndex]->pos().y() - 5
                    );
            });

    if (onFinished)
        connect(anim, &QVariantAnimation::finished, this, onFinished);

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}


void MainWindow::animateMenuSelection(int index)
{
    if (!battleCursorSprite) return;

    // cursor sits slightly above the anchor
    int targetY = battleMenuOptions[index]->pos().y() - 5;

    QVariantAnimation *anim = new QVariantAnimation(this);
    anim->setDuration(80);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    // bounce: start at target, go up 3px, fall back
    anim->setStartValue(targetY);
    anim->setKeyValueAt(0.4, targetY - 3);
    anim->setEndValue(targetY);

    connect(anim, &QVariantAnimation::valueChanged, this,
            [=](const QVariant &value)
            {
                battleCursorSprite->setY(value.toInt());
            });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}


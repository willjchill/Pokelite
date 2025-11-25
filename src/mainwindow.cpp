#include "mainwindow.h"
#include <QDebug>
#include <QBrush>
#include <QPen>
#include <QPainterPath>
#include <QApplication>
#include <QShowEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), gamepadThread(nullptr)
{
    //
    // OVERWORLD WINDOW = 480 x 272 (GBA-style, same as battle)
    //
    setFixedSize(480, 272);

    // Create scene first (we will set the real rect after loading background)
    scene = new QGraphicsScene(this);
    // Set focus policy to ensure keyboard input works
    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    // Scene
    scene = new QGraphicsScene(0, 0, 380, 639, this);

    // View
    view = new QGraphicsView(scene, this);
    view->setFixedSize(480, 272);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setFocusPolicy(Qt::StrongFocus);

    // IMPORTANT: show the view as the central widget
    setCentralWidget(view);

    // Background map
    background = scene->addPixmap(QPixmap(":/assets/background.png"));
    background->setZValue(0);

    // If background loaded, set scene rect to match its size
    if (!background->pixmap().isNull()) {
        scene->setSceneRect(0, 0,
                            background->pixmap().width(),
                            background->pixmap().height());
    } else {
        // Fallback if background fails
        scene->setSceneRect(0, 0, 380, 639);
        qDebug() << "WARNING: background.png missing, using default scene size.";
    }

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
    // Start somewhere reasonable (center-ish); you can adjust
    player->setPos(160, 300);

    // Initial camera lock on player
    updateCamera();

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

    // Initialize gamepad thread
    gamepadThread = new GamepadThread("/dev/input/event1", this);
    connect(gamepadThread, &GamepadThread::inputReceived, this, &MainWindow::handleGamepadInput);
    gamepadThread->start();
}

MainWindow::~MainWindow()
{
    if (gamepadThread) {
        gamepadThread->stop();
        delete gamepadThread;
    }
}

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
// CAMERA FOLLOW
// ---------------------------------------------------------

void MainWindow::updateCamera()
{
    if (!view || !player || !scene)
        return;

    QRectF mapRect = scene->sceneRect();

    // If map is smaller than view, just center on map
    const qreal viewW = 480;
    const qreal viewH = 272;

    if (mapRect.width() <= viewW && mapRect.height() <= viewH) {
        view->centerOn(mapRect.center());
        return;
    }

    // Target camera center = player center
    qreal targetX = player->x() + player->boundingRect().width()  / 2.0;
    qreal targetY = player->y() + player->boundingRect().height() / 2.0;

    // Compute allowed min/max center positions so we don't show outside map
    qreal halfW = viewW / 2.0;
    qreal halfH = viewH / 2.0;

    qreal minCenterX = mapRect.left()  + halfW;
    qreal maxCenterX = mapRect.right() - halfW;
    qreal minCenterY = mapRect.top()   + halfH;
    qreal maxCenterY = mapRect.bottom()- halfH;

    if (mapRect.width() <= viewW) {
        // Can't scroll horizontally, freeze in middle
        targetX = mapRect.center().x();
    } else {
        if (targetX < minCenterX) targetX = minCenterX;
        if (targetX > maxCenterX) targetX = maxCenterX;
    }

    if (mapRect.height() <= viewH) {
        // Can't scroll vertically, freeze in middle
        targetY = mapRect.center().y();
    } else {
        if (targetY < minCenterY) targetY = minCenterY;
        if (targetY > maxCenterY) targetY = maxCenterY;
    }

    view->centerOn(targetX, targetY);
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

    // Camera follow after movement
    updateCamera();

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

        // keep camera updated even when you stop
        updateCamera();
    }
}

void MainWindow::clampPlayer()
{
    QRectF bounds = player->boundingRect();
    QPointF pos = player->pos();

    QRectF mapRect = scene->sceneRect();

    if (pos.x() < mapRect.left())
        pos.setX(mapRect.left());
    if (pos.x() > mapRect.right() - bounds.width())
        pos.setX(mapRect.right() - bounds.width());

    if (pos.y() < mapRect.top())
        pos.setY(mapRect.top());
    if (pos.y() > mapRect.bottom() - bounds.height())
        pos.setY(mapRect.bottom() - bounds.height());

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

    // Ensure focus for input
    setFocus();
    view->setFocus();

    // 4. Fade-in
    battleZoomReveal();

    animateBattleEntrances();

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
    fullBattleText  = "What will BULBASAUR do?";
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
    //

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

    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        animateCommandSelection(battleMenuIndex);
        return;   // prevent moving cursor after selecting
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

void MainWindow::animateCommandSelection(int index)
{
    // 1. Cursor bounce animation
    if (battleCursorSprite)
    {
        QVariantAnimation *cursorBounce = new QVariantAnimation(this);
        cursorBounce->setDuration(120);
        cursorBounce->setStartValue(battleCursorSprite->y());
        cursorBounce->setKeyValueAt(0.5, battleCursorSprite->y() - 6); // bounce up
        cursorBounce->setEndValue(battleCursorSprite->y());
        cursorBounce->setEasingCurve(QEasingCurve::OutCubic);

        connect(cursorBounce, &QVariantAnimation::valueChanged, this,
                [&](const QVariant &v) {
                    battleCursorSprite->setY(v.toReal());
                }
                );

        cursorBounce->start(QAbstractAnimation::DeleteWhenStopped);
    }


    // 2. Flash selected option (white flash)
    QGraphicsRectItem *flash = new QGraphicsRectItem();
    flash->setRect(
        battleMenuOptions[index]->pos().x() - 5,
        battleMenuOptions[index]->pos().y() - 2,
        60, 20   // wide enough to highlight option
        );
    flash->setBrush(Qt::white);
    flash->setPen(Qt::NoPen);
    flash->setOpacity(0.0);
    flash->setZValue(10);
    battleScene->addItem(flash);

    QVariantAnimation *flashAnim = new QVariantAnimation(this);
    flashAnim->setDuration(180);
    flashAnim->setStartValue(0.0);
    flashAnim->setKeyValueAt(0.3, 1.0);
    flashAnim->setEndValue(0.0);

    connect(flashAnim, &QVariantAnimation::valueChanged, this,
            [flash](const QVariant &v) {
                flash->setOpacity(v.toReal());
            }
            );

    connect(flashAnim, &QVariantAnimation::finished, this,
            [flash]() {
                flash->scene()->removeItem(flash);
                delete flash;
            }
            );

    flashAnim->start(QAbstractAnimation::DeleteWhenStopped);


    // 3. Slide command menu OUT of screen
    QVariantAnimation *slideOut = new QVariantAnimation(this);
    slideOut->setDuration(260);
    slideOut->setStartValue(commandBoxSprite->y());
    slideOut->setEndValue(commandBoxSprite->y() + 160); // slide downward
    slideOut->setEasingCurve(QEasingCurve::InQuad);

    connect(slideOut, &QVariantAnimation::valueChanged, this,
            [&](const QVariant &v) {
                qreal newY = v.toReal();
                commandBoxSprite->setY(newY);

                // Update invisible anchors
                for (int i = 0; i < battleMenuOptions.size(); i++) {
                    battleMenuOptions[i]->setY(
                        newY + (i < 2 ? 18 : 42) * commandBoxSprite->scale()
                        );
                }

                // Update cursor
                battleCursorSprite->setY(
                    battleMenuOptions[battleMenuIndex]->y() - 5
                    );
            }
            );

    slideOut->start(QAbstractAnimation::DeleteWhenStopped);


    // 4. Replace dialogue text after menu disappears
    QTimer::singleShot(260, this, [=]() {

        QString commands[4] = {
            "You chose FIGHT!",
            "You opened your BAG...",
            "You check your POK\u00e9MON...",
            "You attempt to RUN..."
        };

        fullBattleText = commands[index];
        battleTextItem->setPlainText("");
        battleTextIndex = 0;

        battleTextTimer.start(22); // typewriter
    });
}

void MainWindow::animateBattleEntrances()
{
    if (!battleTrainerItem || !battleEnemyItem) return;

    //
    // STEP 1 — CREATE CIRCLE MASK THAT OPENS FROM CENTER
    //
    QGraphicsPathItem *mask = new QGraphicsPathItem();
    mask->setBrush(Qt::black);
    mask->setPen(Qt::NoPen);
    mask->setZValue(9999);
    battleScene->addItem(mask);

    QPainterPath fullScreen;
    fullScreen.addRect(0, 0, 480, 272);

    // Circle reveal animation
    QVariantAnimation *circleAnim = new QVariantAnimation(this);
    circleAnim->setDuration(900);
    circleAnim->setStartValue(0.0);     // tiny hole (fully black)
    circleAnim->setEndValue(450.0);     // fully open
    circleAnim->setEasingCurve(QEasingCurve::OutCubic);

    connect(circleAnim, &QVariantAnimation::valueChanged, this,
            [=](const QVariant &v)
            {
                qreal r = v.toReal();
                qreal cx = 240;   // center
                qreal cy = 136;

                QPainterPath hole;
                hole.addEllipse(cx - r, cy - r, r * 2, r * 2);

                QPainterPath maskPath = fullScreen.subtracted(hole);
                mask->setPath(maskPath);
            });

    connect(circleAnim, &QVariantAnimation::finished, this,
            [=]() {
                battleScene->removeItem(mask);
                delete mask;
            });

    circleAnim->start(QAbstractAnimation::DeleteWhenStopped);


    //
    // STEP 2 — PREPARE SPRITES OFFSCREEN
    //
    QPointF playerFinal = battleTrainerItem->pos();
    QPointF enemyFinal  = battleEnemyItem->pos();

    battleTrainerItem->setX(playerFinal.x() - 500);  // slide from left
    battleEnemyItem->setX(enemyFinal.x() + 500);     // slide from right


    //
    // STEP 3 — PLAYER SLIDE IN
    //
    QVariantAnimation *playerSlide = new QVariantAnimation(this);
    playerSlide->setDuration(750);
    playerSlide->setStartValue(battleTrainerItem->x());
    playerSlide->setEndValue(playerFinal.x());
    playerSlide->setEasingCurve(QEasingCurve::OutBack);

    connect(playerSlide, &QVariantAnimation::valueChanged, this,
            [&](const QVariant &v) {
                battleTrainerItem->setX(v.toReal());
            });


    //
    // STEP 4 — ENEMY SLIDE IN
    //
    QVariantAnimation *enemySlide = new QVariantAnimation(this);
    enemySlide->setDuration(750);
    enemySlide->setStartValue(battleEnemyItem->x());
    enemySlide->setEndValue(enemyFinal.x());
    enemySlide->setEasingCurve(QEasingCurve::OutBack);

    connect(enemySlide, &QVariantAnimation::valueChanged, this,
            [&](const QVariant &v) {
                battleEnemyItem->setX(v.toReal());
            });


    //
    // STEP 5 — Start sliding EXACTLY AS THE CIRCLE OPENS
    //
    QTimer::singleShot(100, this, [=]() {
        playerSlide->start(QAbstractAnimation::DeleteWhenStopped);
        enemySlide->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // Ensure focus when window is shown
    setFocus();
    view->setFocus();
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

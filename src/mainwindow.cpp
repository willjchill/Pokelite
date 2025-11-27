#include "mainwindow.h"
#include <QDebug>
#include <QBrush>
#include <QPen>
#include <QPainterPath>
#include <QApplication>

// ============================================================
// CONSTRUCTOR
// ============================================================
#include <QShowEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), gamepadThread(nullptr)
{
    setFixedSize(480, 272);

    scene = new QGraphicsScene(0, 0, 480, 272, this);
      
    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    view = new QGraphicsView(scene, this);
    view->setFixedSize(480, 272);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setFocusPolicy(Qt::StrongFocus);

    setCentralWidget(view);
    setFocus();
    view->setFocus();

    loadAnimations();

    currentDirection = "front";
    frameIndex = 1;

    connect(&animationTimer, &QTimer::timeout, this, [this]() {
        auto &frames = animations[currentDirection];
        frameIndex = (frameIndex + 1) % frames.size();
        player->setPixmap(frames[frameIndex]);
    });

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

    // Load default map
    loadMap("route1");
}

MainWindow::~MainWindow()
{
    if (gamepadThread) {
        gamepadThread->stop();
        delete gamepadThread;
    }
}


// ============================================================
// MAP SYSTEM
// ============================================================

void MainWindow::loadMap(const QString &name)
{
    currentMapName = name;
    currentMap     = MapLoader::load(name);
    applyMap(currentMap);
}

void MainWindow::applyMap(const MapData &m)
{
    scene->clear();

    background = scene->addPixmap(QPixmap(m.background));
    background->setZValue(0);

    if (!background->pixmap().isNull()) {
        scene->setSceneRect(
            0, 0,
            background->pixmap().width(),
            background->pixmap().height()
            );
    }

    // Load masks
    collisionMask = QImage(m.collision);
    tallGrassMask = QImage(m.tallgrass);
    exitMask      = QImage(m.exitMask);

    // Recreate player
    player = scene->addPixmap(animations[currentDirection][frameIndex]);
    player->setZValue(1);
    player->setPos(m.playerSpawn);

    updateCamera();
}

QString MainWindow::detectExitAtPlayerPosition()
{
    if (exitMask.isNull()) return "";

    int px = player->x() + player->boundingRect().width()/2;
    int py = player->y() + player->boundingRect().height() - 4;

    if (px < 0 || py < 0 || px >= exitMask.width() || py >= exitMask.height())
        return "";

    QColor c = exitMask.pixelColor(px, py);

    // EXIT COLORS:
    if (c.red() > 200 && c.green() < 50 && c.blue() < 50)
        return "house1_door";
    if (c.blue() > 200 && c.red() < 50 && c.green() < 50)
        return "cave_entrance";
    if (c.green() > 200 && c.red() < 50 && c.blue() < 50)
        return "exit_door";

    return "";
}

void MainWindow::checkForMapExit()
{
    QString id = detectExitAtPlayerPosition();
    if (id.isEmpty()) return;

    if (currentMap.exits.contains(id)) {
        loadMap(currentMap.exits[id]);
    }
}


// ============================================================
// OVERWORLD MOVEMENT + CAMERA (UNCHANGED)
// ============================================================

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

bool MainWindow::isSolidPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= collisionMask.width() ||
        y >= collisionMask.height())
        return true;

    QColor p = collisionMask.pixelColor(x, y);
    return (p.red() < 30 && p.green() < 30 && p.blue() < 30);
}

bool MainWindow::isSlowPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= collisionMask.width() ||
        y >= collisionMask.height())
        return false;

    QColor p = collisionMask.pixelColor(x, y);
    return (p.blue() > 200 && p.red() < 80 && p.green() < 80);
}

bool MainWindow::isGrassPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= tallGrassMask.width() ||
        y >= tallGrassMask.height())
        return false;

    QColor px = tallGrassMask.pixelColor(x, y);
    return (px.red() > 200 && px.blue() > 200 && px.green() < 100);
}

void MainWindow::updateCamera()
{
    if (!view || !player || !scene)
        return;

    QRectF mapRect = scene->sceneRect();
    const qreal viewW = 480, viewH = 272;

    qreal targetX = player->x() + player->boundingRect().width()/2;
    qreal targetY = player->y() + player->boundingRect().height()/2;

    qreal halfW = viewW/2;
    qreal halfH = viewH/2;

    qreal minCX = mapRect.left() + halfW;
    qreal maxCX = mapRect.right() - halfW;
    qreal minCY = mapRect.top() + halfH;
    qreal maxCY = mapRect.bottom() - halfH;

    if (targetX < minCX) targetX = minCX;
    if (targetX > maxCX) targetX = maxCX;

    if (targetY < minCY) targetY = minCY;
    if (targetY > maxCY) targetY = maxCY;

    view->centerOn(targetX, targetY);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (inBattle) {
        handleBattleKey(event);
        return;
    }
    
    // Handle overworld menu
    if (inOverworldMenu || inOverworldBagMenu || inOverworldPokemonMenu) {
        handleOverworldMenuKey(event);
        return;
    }
    
    // Open menu with 'M' key
    if (event->key() == Qt::Key_M) {
        showOverworldMenu();
        return;
    }

    float dx=0, dy=0;

    switch(event->key()) {
    case Qt::Key_W: dy = -1; currentDirection="back";  break;
    case Qt::Key_S: dy = +1; currentDirection="front"; break;
    case Qt::Key_A: dx = -1; currentDirection="left";  break;
    case Qt::Key_D: dx = +1; currentDirection="right"; break;
    default: return;
    }

    QPointF oldPos = player->pos();
    QPointF tryPos(oldPos.x() + dx * speed,
                   oldPos.y() + dy * speed);

    int px = tryPos.x() + player->boundingRect().width()/2;
    int py = tryPos.y() + player->boundingRect().height() - 4;

    float finalSpeed = speed;
    if (isSlowPixel(px, py))
        finalSpeed = speed * 0.4f;

    tryPos = QPointF(oldPos.x() + dx * finalSpeed,
                     oldPos.y() + dy * finalSpeed);

    px = tryPos.x() + player->boundingRect().width()/2;
    py = tryPos.y() + player->boundingRect().height() - 4;

    if (!isSolidPixel(px, py))
        player->setPos(tryPos);

    updateCamera();

    if (!inBattle && isGrassPixel(px, py))
        tryWildEncounter();

    // NEW:
    checkForMapExit();

    if (!isMoving && (dx!=0 || dy!=0)) {
        isMoving = true;
        frameIndex = 1;
        animationTimer.start(120);
    }

    clampPlayer();
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key()==Qt::Key_W ||
        event->key()==Qt::Key_A ||
        event->key()==Qt::Key_S ||
        event->key()==Qt::Key_D)
    {
        isMoving = false;
        animationTimer.stop();
        frameIndex = 1;
        player->setPixmap(animations[currentDirection][frameIndex]);
        updateCamera();
    }
}

void MainWindow::clampPlayer()
{
    QRectF bounds = player->boundingRect();
    QPointF pos = player->pos();
    QRectF mapRect = scene->sceneRect();

    if (pos.x() < mapRect.left()) pos.setX(mapRect.left());
    if (pos.x() > mapRect.right()-bounds.width()) pos.setX(mapRect.right()-bounds.width());

    if (pos.y() < mapRect.top()) pos.setY(mapRect.top());
    if (pos.y() > mapRect.bottom()-bounds.height()) pos.setY(mapRect.bottom()-bounds.height());

    player->setPos(pos);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // Ensure focus when window is shown - use singleShot to set focus after window is fully shown
    QTimer::singleShot(0, this, [this]() {
        setFocus();
        view->setFocus();
        activateWindow();  // Bring window to front and give it focus
    });
}

// ============================================================
//  INCLUDE BATTLE FILES
// ============================================================

#include "battle/battle_ui_integration.cpp"
#include "battle/battle_animations.cpp"
#include "map/map_loader.cpp"

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

// ============================================================
// OVERWORLD MENU SYSTEM
// ============================================================

void MainWindow::showOverworldMenu()
{
    if (!scene || inOverworldMenu) return;
    
    hideOverworldSubMenu();
    
    QFont font("Pokemon Fire Red", 12, QFont::Bold);
    
    // Create menu box in center of screen
    const qreal boxW = 200, boxH = 100;
    qreal boxX = (480 - boxW) / 2;
    qreal boxY = (272 - boxH) / 2;
    
    overworldMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    overworldMenuRect->setBrush(Qt::white);
    overworldMenuRect->setPen(QPen(Qt::black, 3));
    overworldMenuRect->setZValue(10);
    scene->addItem(overworldMenuRect);
    
    overworldMenuOptions.clear();
    QString options[3] = {"BAG", "POKEMON", "EXIT"};
    for (int i = 0; i < 3; ++i) {
        QGraphicsTextItem *t = new QGraphicsTextItem(options[i]);
        t->setFont(font);
        t->setDefaultTextColor(Qt::black);
        t->setPos(boxX + 20, boxY + 15 + i * 25);
        t->setZValue(11);
        scene->addItem(t);
        overworldMenuOptions.push_back(t);
    }
    
    // Cursor
    QPixmap arrow(":/assets/battle/ui/arrow_cursor.png");
    if (arrow.isNull()) {
        // Fallback: create a simple arrow
        arrow = QPixmap(20, 20);
        arrow.fill(Qt::transparent);
    }
    overworldCursorSprite = scene->addPixmap(arrow);
    overworldCursorSprite->setScale(2.0);
    overworldCursorSprite->setZValue(12);
    
    inOverworldMenu = true;
    overworldMenuIndex = 0;
    updateOverworldMenuCursor();
}

void MainWindow::hideOverworldMenu()
{
    if (overworldMenuRect) {
        scene->removeItem(overworldMenuRect);
        delete overworldMenuRect;
        overworldMenuRect = nullptr;
    }
    
    for (QGraphicsTextItem *t : overworldMenuOptions) {
        if (t) {
            scene->removeItem(t);
            delete t;
        }
    }
    overworldMenuOptions.clear();
    
    if (overworldCursorSprite) {
        scene->removeItem(overworldCursorSprite);
        delete overworldCursorSprite;
        overworldCursorSprite = nullptr;
    }
    
    hideOverworldSubMenu();
    inOverworldMenu = false;
    overworldMenuIndex = 0;
}

void MainWindow::updateOverworldMenuCursor()
{
    if (!overworldCursorSprite) return;
    
    QGraphicsTextItem *target = nullptr;
    QVector<QGraphicsTextItem*> *options = nullptr;
    
    if (inOverworldBagMenu && !overworldBagMenuOptions.isEmpty()) {
        options = &overworldBagMenuOptions;
    } else if (inOverworldPokemonMenu && !overworldPokemonMenuOptions.isEmpty()) {
        options = &overworldPokemonMenuOptions;
    } else if (inOverworldMenu && !overworldMenuOptions.isEmpty()) {
        options = &overworldMenuOptions;
    }
    
    if (options && overworldMenuIndex < options->size()) {
        target = (*options)[overworldMenuIndex];
        overworldCursorSprite->setPos(target->pos().x() - 30, target->pos().y() - 2);
        overworldCursorSprite->setVisible(true);
    } else {
        overworldCursorSprite->setVisible(false);
    }
}

void MainWindow::handleOverworldMenuKey(QKeyEvent *event)
{
    int key = event->key();
    
    if (inOverworldBagMenu || inOverworldPokemonMenu) {
        // Handle submenu navigation
        QVector<QGraphicsTextItem*> *options = inOverworldBagMenu ? &overworldBagMenuOptions : &overworldPokemonMenuOptions;
        int maxIndex = options->size();
        
        if (key == Qt::Key_Up || key == Qt::Key_W) {
            if (overworldMenuIndex > 0)
                overworldMenuIndex--;
        }
        else if (key == Qt::Key_Down || key == Qt::Key_S) {
            if (overworldMenuIndex < maxIndex - 1)
                overworldMenuIndex++;
        }
        else if (key == Qt::Key_Escape || key == Qt::Key_B) {
            hideOverworldSubMenu();
            showOverworldMenu();
            return;
        }
        else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            // BACK is last option
            if (overworldMenuIndex >= maxIndex - 1) {
                hideOverworldSubMenu();
                showOverworldMenu();
                return;
            }
            // TODO: Handle item/pokemon selection
        }
        
        updateOverworldMenuCursor();
        return;
    }
    
    if (!inOverworldMenu) return;
    
    // Main menu navigation
    if (key == Qt::Key_Up || key == Qt::Key_W) {
        if (overworldMenuIndex > 0)
            overworldMenuIndex--;
    }
    else if (key == Qt::Key_Down || key == Qt::Key_S) {
        if (overworldMenuIndex < overworldMenuOptions.size() - 1)
            overworldMenuIndex++;
    }
    else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        overworldMenuSelected(overworldMenuIndex);
        return;
    }
    else if (key == Qt::Key_Escape || key == Qt::Key_M) {
        hideOverworldMenu();
        return;
    }
    
    updateOverworldMenuCursor();
}

void MainWindow::overworldMenuSelected(int index)
{
    if (index == 0) { // BAG
        showOverworldBagMenu();
    } else if (index == 1) { // POKEMON
        showOverworldPokemonMenu();
    } else if (index == 2) { // EXIT
        hideOverworldMenu();
    }
}

void MainWindow::showOverworldBagMenu()
{
    if (!scene || !gamePlayer) return;
    
    hideOverworldMenu();
    hideOverworldSubMenu();
    
    const auto& items = gamePlayer->getBag().getItems();
    QFont font("Pokemon Fire Red", 10, QFont::Bold);
    
    // Create menu box
    const qreal boxW = 300, boxH = 200;
    qreal boxX = (480 - boxW) / 2;
    qreal boxY = (272 - boxH) / 2;
    
    overworldBagMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    overworldBagMenuRect->setBrush(Qt::white);
    overworldBagMenuRect->setPen(QPen(Qt::black, 3));
    overworldBagMenuRect->setZValue(10);
    scene->addItem(overworldBagMenuRect);
    
    overworldBagMenuOptions.clear();
    int itemCount = 0;
    for (size_t i = 0; i < items.size() && itemCount < 8; ++i) {
        if (items[i].getQuantity() > 0) {
            QString label = capitalizeFirst(QString::fromStdString(items[i].getName())) + 
                           " x" + QString::number(items[i].getQuantity());
            QGraphicsTextItem *t = new QGraphicsTextItem(label);
            t->setFont(font);
            t->setDefaultTextColor(Qt::black);
            t->setPos(boxX + 15, boxY + 15 + itemCount * 22);
            t->setZValue(11);
            scene->addItem(t);
            overworldBagMenuOptions.push_back(t);
            itemCount++;
        }
    }
    
    // Add BACK option
    QGraphicsTextItem *back = new QGraphicsTextItem("BACK");
    back->setFont(font);
    back->setDefaultTextColor(Qt::black);
    back->setPos(boxX + 15, boxY + 15 + itemCount * 22);
    back->setZValue(11);
    scene->addItem(back);
    overworldBagMenuOptions.push_back(back);
    
    inOverworldBagMenu = true;
    overworldMenuIndex = 0;
    updateOverworldMenuCursor();
}

void MainWindow::showOverworldPokemonMenu()
{
    if (!scene || !gamePlayer) return;
    
    hideOverworldMenu();
    hideOverworldSubMenu();
    
    const auto& team = gamePlayer->getTeam();
    QFont font("Pokemon Fire Red", 10, QFont::Bold);
    
    // Create menu box
    const qreal boxW = 300, boxH = 200;
    qreal boxX = (480 - boxW) / 2;
    qreal boxY = (272 - boxH) / 2;
    
    overworldPokemonMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    overworldPokemonMenuRect->setBrush(Qt::white);
    overworldPokemonMenuRect->setPen(QPen(Qt::black, 3));
    overworldPokemonMenuRect->setZValue(10);
    scene->addItem(overworldPokemonMenuRect);
    
    overworldPokemonMenuOptions.clear();
    int activeIndex = gamePlayer->getActivePokemonIndex();
    for (size_t i = 0; i < team.size() && i < 6; ++i) {
        QString name = capitalizeFirst(QString::fromStdString(team[i].getName()));
        QString status = team[i].isFainted() ? "FAINTED" : (static_cast<int>(i) == activeIndex ? "ACTIVE" : "");
        QString label = name + " Lv" + QString::number(team[i].getLevel()) + 
                       "\nHP " + QString::number(team[i].getCurrentHP()) + "/" + QString::number(team[i].getMaxHP());
        if (!status.isEmpty()) {
            label += " " + status;
        }
        
        QGraphicsTextItem *t = new QGraphicsTextItem(label);
        t->setFont(font);
        t->setDefaultTextColor(Qt::black);
        t->setPos(boxX + 15, boxY + 15 + i * 30);
        t->setZValue(11);
        scene->addItem(t);
        overworldPokemonMenuOptions.push_back(t);
    }
    
    // Add BACK option
    QGraphicsTextItem *back = new QGraphicsTextItem("BACK");
    back->setFont(font);
    back->setDefaultTextColor(Qt::black);
    back->setPos(boxX + 15, boxY + 15 + team.size() * 30);
    back->setZValue(11);
    scene->addItem(back);
    overworldPokemonMenuOptions.push_back(back);
    
    inOverworldPokemonMenu = true;
    overworldMenuIndex = 0;
    updateOverworldMenuCursor();
}

void MainWindow::hideOverworldSubMenu()
{
    if (overworldBagMenuRect) {
        scene->removeItem(overworldBagMenuRect);
        delete overworldBagMenuRect;
        overworldBagMenuRect = nullptr;
    }
    
    for (QGraphicsTextItem *t : overworldBagMenuOptions) {
        if (t) {
            scene->removeItem(t);
            delete t;
        }
    }
    overworldBagMenuOptions.clear();
    
    if (overworldPokemonMenuRect) {
        scene->removeItem(overworldPokemonMenuRect);
        delete overworldPokemonMenuRect;
        overworldPokemonMenuRect = nullptr;
    }
    
    for (QGraphicsTextItem *t : overworldPokemonMenuOptions) {
        if (t) {
            scene->removeItem(t);
            delete t;
        }
    }
    overworldPokemonMenuOptions.clear();
    
    inOverworldBagMenu = false;
    inOverworldPokemonMenu = false;
    overworldMenuIndex = 0;
}

QString MainWindow::capitalizeFirst(const QString& str) {
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1).toLower();
}

#include "overworld.h"
#include <QDebug>
#include <QBrush>
#include <QPen>
#include <QRandomGenerator>
#include <QFont>
#include <QPainter>
#include <QPolygonF>

Overworld::Overworld(QGraphicsScene *scene, QGraphicsView *view)
    : QObject(nullptr), scene(scene), view(view)
{
    loadAnimations();

    connect(&animationTimer, &QTimer::timeout, this, [this]() {
        auto &frames = animations[currentDirection];
        frameIndex = (frameIndex + 1) % frames.size();
        player->setPixmap(frames[frameIndex]);
    });
}

Overworld::~Overworld()
{
    // Cleanup handled by Qt parent system
}

void Overworld::loadMap(const QString &name)
{
    currentMapName = name;
    currentMap = MapLoader::load(name);
    applyMap(currentMap);
}

void Overworld::applyMap(const MapData &m)
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
    exitMask = QImage(m.exitMask);

    // Recreate player
    player = scene->addPixmap(animations[currentDirection][frameIndex]);
    player->setZValue(1);
    player->setPos(m.playerSpawn);

    updateCamera();
}

QString Overworld::detectExitAtPlayerPosition()
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

void Overworld::checkForMapExit()
{
    QString id = detectExitAtPlayerPosition();
    if (id.isEmpty()) return;

    if (currentMap.exits.contains(id)) {
        loadMap(currentMap.exits[id]);
    }
}

void Overworld::loadAnimations()
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

bool Overworld::isSolidPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= collisionMask.width() ||
        y >= collisionMask.height())
        return true;

    QColor p = collisionMask.pixelColor(x, y);
    return (p.red() < 30 && p.green() < 30 && p.blue() < 30);
}

bool Overworld::isSlowPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= collisionMask.width() ||
        y >= collisionMask.height())
        return false;

    QColor p = collisionMask.pixelColor(x, y);
    return (p.blue() > 200 && p.red() < 80 && p.green() < 80);
}

bool Overworld::isGrassPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= tallGrassMask.width() ||
        y >= tallGrassMask.height())
        return false;

    QColor px = tallGrassMask.pixelColor(x, y);
    return (px.red() > 200 && px.blue() > 200 && px.green() < 100);
}

void Overworld::updateCamera()
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

void Overworld::handleMovementInput(QKeyEvent *event)
{
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

    if (isGrassPixel(px, py))
        tryWildEncounter();

    checkForMapExit();

    if (!isMoving && (dx!=0 || dy!=0)) {
        isMoving = true;
        frameIndex = 1;
        animationTimer.start(120);
    }

    clampPlayer();
}

void Overworld::handleKeyRelease(QKeyEvent *event)
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

void Overworld::clampPlayer()
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

void Overworld::tryWildEncounter()
{
    // Random chance for wild encounter (2% chance per step)
    if (QRandomGenerator::global()->bounded(100) < 2) {
        // Signal that a wild encounter should start
        // This will be handled by MainWindow
        emit wildEncounterTriggered();
    }
}

void Overworld::setPlayerDirection(const QString &direction)
{
    currentDirection = direction;
}

void Overworld::showOverworldMenu()
{
    if (!scene || inOverworldMenu) return;
    
    hideOverworldSubMenu();
    
    QFont font("Pokemon Fire Red", 12, QFont::Bold);
    
    // Create menu box in center of view (not scene)
    const qreal boxW = 200, boxH = 100;
    // Get view center in scene coordinates
    QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();
    qreal boxX = viewRect.center().x() - boxW / 2;
    qreal boxY = viewRect.center().y() - boxH / 2;
    
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
        // Create a simple visible arrow if the image doesn't exist
        arrow = QPixmap(20, 20);
        arrow.fill(Qt::transparent);
        QPainter painter(&arrow);
        painter.setPen(QPen(Qt::black, 2));
        painter.setBrush(Qt::black);
        QPolygonF arrowShape;
        arrowShape << QPointF(0, 10) << QPointF(15, 0) << QPointF(15, 7) 
                   << QPointF(20, 7) << QPointF(20, 13) << QPointF(15, 13) << QPointF(15, 20);
        painter.drawPolygon(arrowShape);
    }
    overworldCursorSprite = scene->addPixmap(arrow);
    overworldCursorSprite->setScale(2.0);
    overworldCursorSprite->setZValue(15); // Higher z-value to ensure visibility
    overworldCursorSprite->setVisible(true);
    
    inOverworldMenu = true;
    overworldMenuIndex = 0;
    updateOverworldMenuCursor();
}

void Overworld::hideOverworldMenu()
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

void Overworld::updateOverworldMenuCursor()
{
    if (!overworldCursorSprite) return;
    
    QGraphicsTextItem *target = nullptr;
    QVector<QGraphicsTextItem*> *options = nullptr;
    
    if (inOverworldPokemonListMenu && !overworldPokemonListMenuOptions.isEmpty()) {
        options = &overworldPokemonListMenuOptions;
    } else if (inOverworldBagMenu && !overworldBagMenuOptions.isEmpty()) {
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

void Overworld::handleOverworldMenuKey(QKeyEvent *event)
{
    int key = event->key();
    
    // Handle Pokemon list menu (when selecting a Pokemon for a slot)
    if (inOverworldPokemonListMenu) {
        int maxIndex = overworldPokemonListMenuOptions.size();
        
        if (key == Qt::Key_Up || key == Qt::Key_W) {
            if (overworldMenuIndex > 0)
                overworldMenuIndex--;
        }
        else if (key == Qt::Key_Down || key == Qt::Key_S) {
            if (overworldMenuIndex < maxIndex - 1)
                overworldMenuIndex++;
        }
        else if (key == Qt::Key_Escape || key == Qt::Key_B) {
            // Go back to slot selection menu - refresh it to show updated team
            if (overworldPokemonListMenuRect) {
                scene->removeItem(overworldPokemonListMenuRect);
                delete overworldPokemonListMenuRect;
                overworldPokemonListMenuRect = nullptr;
            }
            for (QGraphicsTextItem *t : overworldPokemonListMenuOptions) {
                if (t) {
                    scene->removeItem(t);
                    delete t;
                }
            }
            overworldPokemonListMenuOptions.clear();
            
            // Refresh the slot menu to show updated team
            showOverworldPokemonMenu();
            return;
        }
        else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            overworldPokemonListSelected(overworldMenuIndex);
            return;
        }
        
        updateOverworldMenuCursor();
        return;
    }
    
    if (inOverworldBagMenu || inOverworldPokemonMenu) {
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
            // Cursor will be shown by showOverworldMenu
            return;
        }
        else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            // Handle Pokemon slot selection
            if (inOverworldPokemonMenu) {
                if (overworldMenuIndex < 6) {
                    // Slot selected - show Pokemon list
                    overworldPokemonSelected(overworldMenuIndex);
                }
                return;
            }
            // TODO: Handle item selection in overworld bag menu
        }
        
        updateOverworldMenuCursor();
        return;
    }
    
    if (!inOverworldMenu) return;
    
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
        // Hide cursor after selection (menu will be hidden or submenu shown)
        if (overworldCursorSprite) {
            overworldCursorSprite->setVisible(false);
        }
        return;
    }
    else if (key == Qt::Key_Escape || key == Qt::Key_M) {
        hideOverworldMenu();
        return;
    }
    
    updateOverworldMenuCursor();
}

void Overworld::overworldMenuSelected(int index)
{
    if (index == 0) {
        showOverworldBagMenu();
    } else if (index == 1) {
        showOverworldPokemonMenu();
    } else if (index == 2) {
        hideOverworldMenu();
    }
}

void Overworld::showOverworldBagMenu()
{
    if (!scene || !gamePlayer) return;
    
    hideOverworldMenu();
    hideOverworldSubMenu();
    
    const auto& items = gamePlayer->getBag().getItems();
    QFont font("Pokemon Fire Red", 10, QFont::Bold);
    
    const qreal boxW = 300, boxH = 200;
    // Get view center in scene coordinates
    QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();
    qreal boxX = viewRect.center().x() - boxW / 2;
    qreal boxY = viewRect.center().y() - boxH / 2;
    
    overworldBagMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    overworldBagMenuRect->setBrush(Qt::white);
    overworldBagMenuRect->setPen(QPen(Qt::black, 3));
    overworldBagMenuRect->setZValue(10);
    scene->addItem(overworldBagMenuRect);
    
    overworldBagMenuOptions.clear();
    int itemCount = 0;
    for (size_t i = 0; i < items.size() && itemCount < 8; ++i) {
        if (items[i].getQuantity() > 0) {
            QString name = QString::fromStdString(items[i].getName());
            if (!name.isEmpty()) {
                name = name[0].toUpper() + name.mid(1).toLower();
            }
            QString label = name + " x" + QString::number(items[i].getQuantity());
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
    
    QGraphicsTextItem *back = new QGraphicsTextItem("BACK");
    back->setFont(font);
    back->setDefaultTextColor(Qt::black);
    back->setPos(boxX + 15, boxY + 15 + itemCount * 22);
    back->setZValue(11);
    scene->addItem(back);
    overworldBagMenuOptions.push_back(back);
    
    // Ensure cursor exists and is visible
    if (!overworldCursorSprite) {
        QPixmap arrow(":/assets/battle/ui/arrow_cursor.png");
        if (arrow.isNull()) {
            arrow = QPixmap(20, 20);
            arrow.fill(Qt::transparent);
            QPainter painter(&arrow);
            painter.setPen(QPen(Qt::black, 2));
            painter.setBrush(Qt::black);
            QPolygonF arrowShape;
            arrowShape << QPointF(0, 10) << QPointF(15, 0) << QPointF(15, 7) 
                       << QPointF(20, 7) << QPointF(20, 13) << QPointF(15, 13) << QPointF(15, 20);
            painter.drawPolygon(arrowShape);
        }
        overworldCursorSprite = scene->addPixmap(arrow);
        overworldCursorSprite->setScale(2.0);
        overworldCursorSprite->setZValue(15);
    }
    overworldCursorSprite->setVisible(true);
    
    inOverworldBagMenu = true;
    overworldMenuIndex = 0;
    updateOverworldMenuCursor();
}

void Overworld::showOverworldPokemonMenu()
{
    if (!scene || !gamePlayer) return;
    
    hideOverworldMenu();
    hideOverworldSubMenu();
    
    const auto& team = gamePlayer->getTeam();
    QFont font("Pokemon Fire Red", 10, QFont::Bold);
    
    const qreal boxW = 300, boxH = 200; // Height for 6 slots
    // Get view center in scene coordinates
    QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();
    qreal boxX = viewRect.center().x() - boxW / 2;
    qreal boxY = viewRect.center().y() - boxH / 2;
    
    overworldPokemonMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    overworldPokemonMenuRect->setBrush(Qt::white);
    overworldPokemonMenuRect->setPen(QPen(Qt::black, 3));
    overworldPokemonMenuRect->setZValue(10);
    scene->addItem(overworldPokemonMenuRect);
    
    overworldPokemonMenuOptions.clear();
    
    // Show all 6 slots
    for (int i = 0; i < 6; ++i) {
        QString label;
        QColor textColor;
        
        if (i < static_cast<int>(team.size())) {
            // Slot has a Pokemon
            QString name = QString::fromStdString(team[i].getName());
            if (!name.isEmpty()) {
                name = name[0].toUpper() + name.mid(1).toLower();
            }
            label = name + " Lv" + QString::number(team[i].getLevel()) + 
                   " HP " + QString::number(team[i].getCurrentHP()) + "/" + QString::number(team[i].getMaxHP());
            
            // First slot (index 0) should be red
            if (i == 0) {
                textColor = QColor(200, 0, 0); // Red for first slot
            } else {
                textColor = Qt::black; // Black for other slots
            }
        } else {
            // Empty slot
            label = "(EMPTY SLOT)";
            textColor = QColor(150, 150, 150); // Grey for empty slots
        }
        
        QGraphicsTextItem *t = new QGraphicsTextItem(label);
        t->setFont(font);
        t->setDefaultTextColor(textColor);
        t->setPos(boxX + 15, boxY + 15 + i * 28);
        t->setZValue(11);
        scene->addItem(t);
        overworldPokemonMenuOptions.push_back(t);
    }
    
    // Ensure cursor exists and is visible
    if (!overworldCursorSprite) {
        QPixmap arrow(":/assets/battle/ui/arrow_cursor.png");
        if (arrow.isNull()) {
            arrow = QPixmap(20, 20);
            arrow.fill(Qt::transparent);
            QPainter painter(&arrow);
            painter.setPen(QPen(Qt::black, 2));
            painter.setBrush(Qt::black);
            QPolygonF arrowShape;
            arrowShape << QPointF(0, 10) << QPointF(15, 0) << QPointF(15, 7) 
                       << QPointF(20, 7) << QPointF(20, 13) << QPointF(15, 13) << QPointF(15, 20);
            painter.drawPolygon(arrowShape);
        }
        overworldCursorSprite = scene->addPixmap(arrow);
        overworldCursorSprite->setScale(2.0);
        overworldCursorSprite->setZValue(15);
    }
    overworldCursorSprite->setVisible(true);
    
    inOverworldPokemonMenu = true;
    inOverworldPokemonListMenu = false;
    selectedSlotIndex = -1;
    overworldMenuIndex = 0;
    updateOverworldMenuCursor();
}

void Overworld::hideOverworldSubMenu()
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
    
    if (overworldPokemonListMenuRect) {
        scene->removeItem(overworldPokemonListMenuRect);
        delete overworldPokemonListMenuRect;
        overworldPokemonListMenuRect = nullptr;
    }
    
    for (QGraphicsTextItem *t : overworldPokemonListMenuOptions) {
        if (t) {
            scene->removeItem(t);
            delete t;
        }
    }
    overworldPokemonListMenuOptions.clear();
    
    inOverworldBagMenu = false;
    inOverworldPokemonMenu = false;
    inOverworldPokemonListMenu = false;
    selectedSlotIndex = -1;
    overworldMenuIndex = 0;
}

void Overworld::showOverworldPokemonListMenu(int slotIndex)
{
    if (!scene || !gamePlayer) return;
    
    selectedSlotIndex = slotIndex;
    inOverworldPokemonListMenu = true;
    
    const auto& team = gamePlayer->getTeam();
    QFont font("Pokemon Fire Red", 10, QFont::Bold);
    
    const qreal boxW = 300, boxH = 250;
    // Get view center in scene coordinates
    QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();
    qreal boxX = viewRect.center().x() - boxW / 2;
    qreal boxY = viewRect.center().y() - boxH / 2;
    
    // Hide the slot menu temporarily (we'll refresh it when going back)
    if (overworldPokemonMenuRect) {
        overworldPokemonMenuRect->setVisible(false);
    }
    for (QGraphicsTextItem *t : overworldPokemonMenuOptions) {
        if (t) t->setVisible(false);
    }
    
    overworldPokemonListMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    overworldPokemonListMenuRect->setBrush(Qt::white);
    overworldPokemonListMenuRect->setPen(QPen(Qt::black, 3));
    overworldPokemonListMenuRect->setZValue(10);
    scene->addItem(overworldPokemonListMenuRect);
    
    overworldPokemonListMenuOptions.clear();
    
    // Show all Pokemon in team
    for (size_t i = 0; i < team.size(); ++i) {
        QString name = QString::fromStdString(team[i].getName());
        if (!name.isEmpty()) {
            name = name[0].toUpper() + name.mid(1).toLower();
        }
        
        QString label = name + " Lv" + QString::number(team[i].getLevel()) + 
                       " HP " + QString::number(team[i].getCurrentHP()) + "/" + QString::number(team[i].getMaxHP());
        
        QGraphicsTextItem *t = new QGraphicsTextItem(label);
        t->setFont(font);
        t->setDefaultTextColor(Qt::black);
        t->setPos(boxX + 15, boxY + 15 + static_cast<int>(i) * 25);
        t->setZValue(11);
        scene->addItem(t);
        overworldPokemonListMenuOptions.push_back(t);
    }
    
    QGraphicsTextItem *back = new QGraphicsTextItem("BACK");
    back->setFont(font);
    back->setDefaultTextColor(Qt::black);
    back->setPos(boxX + 15, boxY + 15 + static_cast<int>(team.size()) * 25);
    back->setZValue(11);
    scene->addItem(back);
    overworldPokemonListMenuOptions.push_back(back);
    
    overworldMenuIndex = 0;
    updateOverworldMenuCursor();
}

void Overworld::overworldPokemonSelected(int index)
{
    if (!gamePlayer) return;
    
    // index is the slot index (0-5)
    if (index < 0 || index >= 6) {
        return;
    }
    
    // Show the Pokemon list menu for this slot
    showOverworldPokemonListMenu(index);
}

void Overworld::overworldPokemonListSelected(int index)
{
    if (!gamePlayer || selectedSlotIndex < 0 || selectedSlotIndex >= 6) return;
    
    const auto& team = gamePlayer->getTeam();
    
    // Check if BACK was selected (last option)
    if (index >= static_cast<int>(team.size())) {
        // Go back to slot selection menu - refresh it to show updated team
        if (overworldPokemonListMenuRect) {
            scene->removeItem(overworldPokemonListMenuRect);
            delete overworldPokemonListMenuRect;
            overworldPokemonListMenuRect = nullptr;
        }
        for (QGraphicsTextItem *t : overworldPokemonListMenuOptions) {
            if (t) {
                scene->removeItem(t);
                delete t;
            }
        }
        overworldPokemonListMenuOptions.clear();
        
        // Refresh the slot menu to show updated team
        showOverworldPokemonMenu();
        return;
    }
    
    // Handle Pokemon selection
    swapOrReplacePokemon(selectedSlotIndex, index);
    
    // Close all menus
    hideOverworldSubMenu();
    if (overworldCursorSprite) {
        overworldCursorSprite->setVisible(false);
    }
}

void Overworld::swapOrReplacePokemon(int slotIndex, int pokemonIndex)
{
    if (!gamePlayer || slotIndex < 0 || slotIndex >= 6 || pokemonIndex < 0) return;
    
    auto& team = gamePlayer->getTeam();
    if (pokemonIndex >= static_cast<int>(team.size())) return;
    
    // If selecting the same Pokemon that's already in that slot, do nothing
    if (slotIndex == pokemonIndex) {
        return;
    }
    
    // Pokemon is in a different slot - swap them
    if (slotIndex < static_cast<int>(team.size())) {
        // Both slots have Pokemon - swap
        std::swap(team[slotIndex], team[pokemonIndex]);
        
        // Update active Pokemon index if needed
        int activeIndex = gamePlayer->getActivePokemonIndex();
        if (activeIndex == slotIndex) {
            gamePlayer->switchPokemon(pokemonIndex);
        } else if (activeIndex == pokemonIndex) {
            gamePlayer->switchPokemon(slotIndex);
        }
    } else {
        // Target slot is empty - move Pokemon from pokemonIndex to slotIndex
        // Since slotIndex >= team.size(), we need to expand the team
        Pokemon pokemonToMove = team[pokemonIndex];
        team.erase(team.begin() + pokemonIndex);
        
        // Resize team to slotIndex + 1, filling with the moved Pokemon
        while (static_cast<int>(team.size()) < slotIndex) {
            // This shouldn't happen in normal gameplay, but handle it
            // We can't create empty Pokemon, so we'll just push the moved one
            break;
        }
        
        // Insert at slotIndex
        if (static_cast<int>(team.size()) == slotIndex) {
            team.push_back(pokemonToMove);
        } else {
            team.insert(team.begin() + slotIndex, pokemonToMove);
        }
        
        // Update active Pokemon index
        int activeIndex = gamePlayer->getActivePokemonIndex();
        if (activeIndex == pokemonIndex) {
            // The moved Pokemon was active - it's now at slotIndex
            gamePlayer->switchPokemon(slotIndex);
        } else if (activeIndex > pokemonIndex) {
            // Active Pokemon shifted left
            gamePlayer->switchPokemon(activeIndex - 1);
        }
    }
}


#include "Menu_OW.h"
#include "../Battle/Battle_logic/Pokemon.h"
#include <QFont>
#include <QPainter>
#include <QPolygonF>
#include <QDebug>
#include <QGraphicsView>

Menu_OW::Menu_OW(QGraphicsScene *scene, QGraphicsView *view, QObject *parent)
    : QObject(parent), scene(scene), view(view)
{
}

Menu_OW::~Menu_OW()
{
    hideMenu();
}

void Menu_OW::showMenu()
{
    if (!scene || !view || inMenu) return;
    
    hideSubMenu();
    
    QFont font("Pokemon Fire Red", 10, QFont::Bold);
    
    // Get view center in scene coordinates (accounting for zoom)
    QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();
    qreal centerX = viewRect.center().x();
    qreal centerY = viewRect.center().y();
    
    const qreal boxW = 150, boxH = 60;
    qreal boxX = centerX - boxW / 2;
    qreal boxY = centerY - boxH / 2;
    
    menuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    menuRect->setBrush(Qt::white);
    menuRect->setPen(QPen(Qt::black, 2));
    menuRect->setZValue(10);
    scene->addItem(menuRect);
    
    menuOptions.clear();
    QString options[3] = {"POKEMON", "PVP BATTLE", "EXIT"};
    for (int i = 0; i < 3; ++i) {
        QGraphicsTextItem *t = new QGraphicsTextItem(options[i]);
        t->setFont(font);
        t->setDefaultTextColor(Qt::black);
        t->setPos(boxX + 15, boxY + 10 + i * 20);
        t->setZValue(11);
        scene->addItem(t);
        menuOptions.push_back(t);
    }
    
    // Cursor
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
    cursorSprite = scene->addPixmap(arrow);
    cursorSprite->setScale(1.5);
    cursorSprite->setZValue(15);
    cursorSprite->setVisible(true);
    
    inMenu = true;
    menuIndex = 0;
    updateCursor();
}

void Menu_OW::hideMenu()
{
    if (menuRect) {
        scene->removeItem(menuRect);
        delete menuRect;
        menuRect = nullptr;
    }
    
    for (QGraphicsTextItem *t : menuOptions) {
        if (t) {
            scene->removeItem(t);
            delete t;
        }
    }
    menuOptions.clear();
    
    if (cursorSprite) {
        scene->removeItem(cursorSprite);
        delete cursorSprite;
        cursorSprite = nullptr;
    }
    
    hideSubMenu();
    inMenu = false;
    menuIndex = 0;
}

void Menu_OW::handleKey(QKeyEvent *event)
{
    int key = event->key();
    
    if (inPokemonListMenu) {
        int maxIndex = pokemonListMenuOptions.size();
        
        if (key == Qt::Key_Up || key == Qt::Key_W) {
            if (menuIndex > 0) menuIndex--;
        }
        else if (key == Qt::Key_Down || key == Qt::Key_S) {
            if (menuIndex < maxIndex - 1) menuIndex++;
        }
        else if (key == Qt::Key_Escape || key == Qt::Key_B) {
            if (pokemonListMenuRect) {
                scene->removeItem(pokemonListMenuRect);
                delete pokemonListMenuRect;
                pokemonListMenuRect = nullptr;
            }
            for (QGraphicsTextItem *t : pokemonListMenuOptions) {
                if (t) {
                    scene->removeItem(t);
                    delete t;
                }
            }
            pokemonListMenuOptions.clear();
            // Hide cursor when going back to Pokemon menu
            if (cursorSprite) {
                cursorSprite->setVisible(false);
            }
            showPokemonMenu();
            return;
        }
        else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            pokemonListSelected(menuIndex);
            return;
        }
        
        updateCursor();
        return;
    }
    
    if (inPokemonMenu) {
        int maxIndex = pokemonMenuOptions.size();
        
        if (key == Qt::Key_Up || key == Qt::Key_W) {
            if (menuIndex > 0) menuIndex--;
        }
        else if (key == Qt::Key_Down || key == Qt::Key_S) {
            if (menuIndex < maxIndex - 1) menuIndex++;
        }
        else if (key == Qt::Key_Escape || key == Qt::Key_B) {
            hideSubMenu();
            showMenu();
            return;
        }
        else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            if (menuIndex < 6) {
                pokemonSelected(menuIndex);
            }
            return;
        }
        
        updateCursor();
        return;
    }
    
    if (!inMenu) return;
    
    if (key == Qt::Key_Up || key == Qt::Key_W) {
        if (menuIndex > 0) menuIndex--;
    }
    else if (key == Qt::Key_Down || key == Qt::Key_S) {
        if (menuIndex < menuOptions.size() - 1) menuIndex++;
    }
    else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        menuSelected(menuIndex);
        return;
    }
    else if (key == Qt::Key_Escape || key == Qt::Key_M) {
        hideMenu();
        emit menuClosed();
        return;
    }
    
    updateCursor();
}

void Menu_OW::menuSelected(int index)
{
    if (index == 0) {
        showPokemonMenu();
    } else if (index == 1) {
        // PvP Battle option
        hideMenu();
        emit pvpBattleRequested();
    } else if (index == 2) {
        hideMenu();
        emit menuClosed();
    }
}

void Menu_OW::showPokemonMenu()
{
    if (!scene || !view || !gamePlayer) return;
    
    hideMenu();
    hideSubMenu();
    
    const auto& team = gamePlayer->getTeam();
    QFont font("Pokemon Fire Red", 8, QFont::Bold);
    
    // Get view center in scene coordinates (accounting for zoom)
    QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();
    qreal centerX = viewRect.center().x();
    qreal centerY = viewRect.center().y();
    
    const qreal boxW = 140, boxH = 100;
    qreal boxX = centerX - boxW / 2;
    qreal boxY = centerY - boxH / 2;
    
    pokemonMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    pokemonMenuRect->setBrush(Qt::white);
    pokemonMenuRect->setPen(QPen(Qt::black, 2));
    pokemonMenuRect->setZValue(10);
    scene->addItem(pokemonMenuRect);
    
    pokemonMenuOptions.clear();
    
    // Slot 0 is always the active Pokemon and should be red
    for (int i = 0; i < 6; ++i) {
        QString label;
        QColor textColor;
        
        if (i < static_cast<int>(team.size())) {
            QString name = QString::fromStdString(team[i].getName());
            if (!name.isEmpty()) {
                name = name[0].toUpper() + name.mid(1).toLower();
            }
            label = name + " L" + QString::number(team[i].getLevel());
            
            // Slot 0 is always red (the active/first Pokemon)
            if (i == 0) {
                textColor = QColor(200, 0, 0);
            } else {
                textColor = Qt::black;
            }
        } else {
            label = "(EMPTY)";
            textColor = QColor(150, 150, 150);
        }
        
        QGraphicsTextItem *t = new QGraphicsTextItem(label);
        t->setFont(font);
        t->setDefaultTextColor(textColor);
        t->setPos(boxX + 8, boxY + 6 + i * 14);
        t->setZValue(11);
        scene->addItem(t);
        pokemonMenuOptions.push_back(t);
    }
    
    if (!cursorSprite) {
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
        cursorSprite = scene->addPixmap(arrow);
        cursorSprite->setScale(1.5);
        cursorSprite->setZValue(15);
    }
    cursorSprite->setVisible(true);
    
    inPokemonMenu = true;
    inPokemonListMenu = false;
    selectedSlotIndex = -1;
    menuIndex = 0;
    updateCursor();
}

void Menu_OW::showPokemonListMenu(int slotIndex)
{
    if (!scene || !view || !gamePlayer) return;
    
    selectedSlotIndex = slotIndex;
    inPokemonListMenu = true;
    
    const auto& team = gamePlayer->getTeam();
    QFont font("Pokemon Fire Red", 8, QFont::Bold);
    
    // Get view center in scene coordinates (accounting for zoom)
    QRectF viewRect = view->mapToScene(view->viewport()->rect()).boundingRect();
    qreal centerX = viewRect.center().x();
    qreal centerY = viewRect.center().y();
    
    const qreal boxW = 140, boxH = 120;
    qreal boxX = centerX - boxW / 2;
    qreal boxY = centerY - boxH / 2;
    
    if (pokemonMenuRect) {
        pokemonMenuRect->setVisible(false);
    }
    for (QGraphicsTextItem *t : pokemonMenuOptions) {
        if (t) t->setVisible(false);
    }
    
    pokemonListMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    pokemonListMenuRect->setBrush(Qt::white);
    pokemonListMenuRect->setPen(QPen(Qt::black, 2));
    pokemonListMenuRect->setZValue(10);
    scene->addItem(pokemonListMenuRect);
    
    pokemonListMenuOptions.clear();
    
    for (size_t i = 0; i < team.size(); ++i) {
        QString name = QString::fromStdString(team[i].getName());
        if (!name.isEmpty()) {
            name = name[0].toUpper() + name.mid(1).toLower();
        }
        
        QString label = name + " L" + QString::number(team[i].getLevel());
        
        QGraphicsTextItem *t = new QGraphicsTextItem(label);
        t->setFont(font);
        t->setDefaultTextColor(Qt::black);
        t->setPos(boxX + 8, boxY + 6 + static_cast<int>(i) * 14);
        t->setZValue(11);
        scene->addItem(t);
        pokemonListMenuOptions.push_back(t);
    }
    
    QGraphicsTextItem *back = new QGraphicsTextItem("BACK");
    back->setFont(font);
    back->setDefaultTextColor(Qt::black);
    back->setPos(boxX + 8, boxY + 6 + static_cast<int>(team.size()) * 14);
    back->setZValue(11);
    scene->addItem(back);
    pokemonListMenuOptions.push_back(back);
    
    menuIndex = 0;
    updateCursor();
}

void Menu_OW::hideSubMenu()
{
    if (pokemonMenuRect) {
        scene->removeItem(pokemonMenuRect);
        delete pokemonMenuRect;
        pokemonMenuRect = nullptr;
    }
    
    for (QGraphicsTextItem *t : pokemonMenuOptions) {
        if (t) {
            scene->removeItem(t);
            delete t;
        }
    }
    pokemonMenuOptions.clear();
    
    if (pokemonListMenuRect) {
        scene->removeItem(pokemonListMenuRect);
        delete pokemonListMenuRect;
        pokemonListMenuRect = nullptr;
    }
    
    for (QGraphicsTextItem *t : pokemonListMenuOptions) {
        if (t) {
            scene->removeItem(t);
            delete t;
        }
    }
    pokemonListMenuOptions.clear();
    
    inPokemonMenu = false;
    inPokemonListMenu = false;
    selectedSlotIndex = -1;
    menuIndex = 0;
    
    // Hide cursor when submenu is closed
    if (cursorSprite) {
        cursorSprite->setVisible(false);
    }
}

void Menu_OW::updateCursor()
{
    if (!cursorSprite) return;
    
    QGraphicsTextItem *target = nullptr;
    QVector<QGraphicsTextItem*> *options = nullptr;
    
    if (inPokemonListMenu && !pokemonListMenuOptions.isEmpty()) {
        options = &pokemonListMenuOptions;
    } else if (inPokemonMenu && !pokemonMenuOptions.isEmpty()) {
        options = &pokemonMenuOptions;
    } else if (inMenu && !menuOptions.isEmpty()) {
        options = &menuOptions;
    }
    
    if (options && menuIndex < options->size()) {
        target = (*options)[menuIndex];
        cursorSprite->setPos(target->pos().x() - 30, target->pos().y() - 2);
        cursorSprite->setVisible(true);
    } else {
        cursorSprite->setVisible(false);
    }
}

void Menu_OW::pokemonSelected(int index)
{
    if (index < 0 || index >= 6) return;
    showPokemonListMenu(index);
}

void Menu_OW::pokemonListSelected(int index)
{
    if (!gamePlayer || selectedSlotIndex < 0 || selectedSlotIndex >= 6) return;
    
    const auto& team = gamePlayer->getTeam();
    
    if (index >= static_cast<int>(team.size())) {
        if (pokemonListMenuRect) {
            scene->removeItem(pokemonListMenuRect);
            delete pokemonListMenuRect;
            pokemonListMenuRect = nullptr;
        }
        for (QGraphicsTextItem *t : pokemonListMenuOptions) {
            if (t) {
                scene->removeItem(t);
                delete t;
            }
        }
        pokemonListMenuOptions.clear();
        // Hide cursor when going back to Pokemon menu
        if (cursorSprite) {
            cursorSprite->setVisible(false);
        }
        showPokemonMenu();
        return;
    }
    
    swapOrReplacePokemon(selectedSlotIndex, index);
    hideSubMenu();
    hideMenu();  // Hide main menu and cursor when done
    emit menuClosed();
}

void Menu_OW::swapOrReplacePokemon(int slotIndex, int pokemonIndex)
{
    if (!gamePlayer || slotIndex < 0 || slotIndex >= 6 || pokemonIndex < 0) return;
    
    auto& team = gamePlayer->getTeam();
    if (pokemonIndex >= static_cast<int>(team.size())) return;
    
    // If trying to move Pokemon to its own slot, do nothing
    if (slotIndex == pokemonIndex) return;
    
    // If both slots are occupied, just swap
    if (slotIndex < static_cast<int>(team.size())) {
        std::swap(team[slotIndex], team[pokemonIndex]);
        
        // Slot 0 is always the active Pokemon - ensure it's set after swap
        gamePlayer->switchPokemon(0);
    } else {
        // Moving to an empty slot
        Pokemon pokemonToMove = team[pokemonIndex];
        team.erase(team.begin() + pokemonIndex);
        
        // After erasing, team.size() has decreased by 1
        // slotIndex should now be team.size() (the next empty slot) or less
        int newSize = static_cast<int>(team.size());
        if (slotIndex > newSize) {
            // slotIndex is now out of bounds, just append
            team.push_back(pokemonToMove);
            slotIndex = newSize; // Update for active index tracking
        } else if (slotIndex == newSize) {
            team.push_back(pokemonToMove);
        } else {
            team.insert(team.begin() + slotIndex, pokemonToMove);
        }
        
        // Slot 0 is always the active Pokemon
        gamePlayer->switchPokemon(0);
    }
}


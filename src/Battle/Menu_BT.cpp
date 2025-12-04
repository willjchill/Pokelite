#include "Menu_BT.h"
#include <QFont>
#include <QDebug>

Menu_BT::Menu_BT(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), scene(scene)
{
}

Menu_BT::~Menu_BT()
{
    hideAllMenus();
}

void Menu_BT::showDialogue(const QString &text)
{
    if (!scene) return;
    
    if (!dialogueText) {
        QFont font("Pokemon Fire Red", 10, QFont::Bold);
        dialogueText = new QGraphicsTextItem();
        dialogueText->setFont(font);
        dialogueText->setDefaultTextColor(Qt::white);
        dialogueText->setPos(18, 200);
        dialogueText->setZValue(3);
        scene->addItem(dialogueText);
    }
    
    dialogueText->setPlainText(text);
}

void Menu_BT::showCommandMenu()
{
    if (!scene) return;
    
    destroyMoveMenu();
    destroyPokemonMenu();
    
    QFont font("Pokemon Fire Red", 10, QFont::Bold);
    
    const qreal boxW = 160, boxH = 64;
    qreal boxX = 270;
    qreal boxY = 191;
    
    commandMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    commandMenuRect->setBrush(Qt::white);
    commandMenuRect->setPen(QPen(Qt::black, 2));
    commandMenuRect->setZValue(3);
    scene->addItem(commandMenuRect);
    
    commandMenuOptions.clear();
    QString options[4] = {"FIGHT", "BAG", "POKEMON", "RUN"};
    int px[4] = {25, 100, 25, 100};
    int py[4] = {18, 18, 42, 42};
    
    for (int i = 0; i < 4; ++i) {
        QGraphicsTextItem *t = new QGraphicsTextItem(options[i]);
        t->setFont(font);
        t->setDefaultTextColor(Qt::black);
        t->setPos(boxX + px[i], boxY + py[i]);
        t->setZValue(4);
        scene->addItem(t);
        commandMenuOptions.push_back(t);
    }
    
    if (!cursorSprite) {
        QPixmap arrow(":/assets/battle/ui/arrow_cursor.png");
        cursorSprite = scene->addPixmap(arrow);
        cursorSprite->setScale(2.0);
        cursorSprite->setZValue(5);
    }
    
    inCommandMenu = true;
    inMoveMenu = false;
    inPokemonMenu = false;
    menuIndex = 0;
    updateCursor();
}

void Menu_BT::showMoveMenu(const QVector<QString> &moves)
{
    if (!scene) return;
    
    destroyCommandMenu();
    
    QFont font("Pokemon Fire Red", 9, QFont::Bold);
    
    const qreal boxW = 240, boxH = 80;
    qreal boxX = 10;
    qreal boxY = 120;
    
    moveMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    moveMenuRect->setBrush(Qt::white);
    moveMenuRect->setPen(QPen(Qt::black, 2));
    moveMenuRect->setZValue(3);
    scene->addItem(moveMenuRect);
    
    moveMenuOptions.clear();
    int numMoves = std::min(4, moves.size() + 1);
    for (int i = 0; i < numMoves; ++i) {
        QString label = (i < moves.size()) ? moves[i] : "BACK";
        
        QGraphicsTextItem *t = new QGraphicsTextItem(label);
        t->setFont(font);
        t->setDefaultTextColor(Qt::black);
        
        int row = i / 2;
        int col = i % 2;
        t->setPos(boxX + 12 + col * 115, boxY + 10 + row * 35);
        t->setZValue(4);
        scene->addItem(t);
        moveMenuOptions.push_back(t);
    }
    
    inMoveMenu = true;
    inCommandMenu = false;
    menuIndex = 0;
    updateCursor();
}

void Menu_BT::showPokemonMenu(const QVector<QString> &pokemon)
{
    if (!scene) return;
    
    destroyCommandMenu();
    
    QFont font("Pokemon Fire Red", 9, QFont::Bold);
    
    const qreal boxW = 240, boxH = 140;
    qreal boxX = 10;
    qreal boxY = 80;
    
    pokemonMenuRect = new QGraphicsRectItem(boxX, boxY, boxW, boxH);
    pokemonMenuRect->setBrush(Qt::white);
    pokemonMenuRect->setPen(QPen(Qt::black, 2));
    pokemonMenuRect->setZValue(3);
    scene->addItem(pokemonMenuRect);
    
    pokemonMenuOptions.clear();
    int numPokemon = std::min(6, pokemon.size() + 1);
    for (int i = 0; i < numPokemon; ++i) {
        QString label = (i < pokemon.size()) ? pokemon[i] : "BACK";
        
        QGraphicsTextItem *t = new QGraphicsTextItem(label);
        t->setFont(font);
        t->setDefaultTextColor(Qt::black);
        
        int row = i / 2;
        int col = i % 2;
        t->setPos(boxX + 12 + col * 115, boxY + 10 + row * 25);
        t->setZValue(4);
        scene->addItem(t);
        pokemonMenuOptions.push_back(t);
    }
    
    inPokemonMenu = true;
    inCommandMenu = false;
    menuIndex = 0;
    updateCursor();
}

void Menu_BT::hideAllMenus()
{
    destroyCommandMenu();
    destroyMoveMenu();
    destroyPokemonMenu();
    
    if (dialogueText) {
        scene->removeItem(dialogueText);
        delete dialogueText;
        dialogueText = nullptr;
    }
    
    if (cursorSprite) {
        scene->removeItem(cursorSprite);
        delete cursorSprite;
        cursorSprite = nullptr;
    }
}

void Menu_BT::handleKey(QKeyEvent *event)
{
    int key = event->key();
    int maxIndex = 0;
    QVector<QGraphicsTextItem*> *options = nullptr;
    
    if (inMoveMenu) {
        options = &moveMenuOptions;
        maxIndex = moveMenuOptions.size();
    } else if (inPokemonMenu) {
        options = &pokemonMenuOptions;
        maxIndex = pokemonMenuOptions.size();
    } else if (inCommandMenu) {
        options = &commandMenuOptions;
        maxIndex = commandMenuOptions.size();
    }
    
    if (!options || maxIndex == 0) return;
    
    if (key == Qt::Key_Left || key == Qt::Key_A) {
        if (menuIndex % 2 == 1) menuIndex--;
    }
    else if (key == Qt::Key_Right || key == Qt::Key_D) {
        if (menuIndex % 2 == 0 && menuIndex + 1 < maxIndex) menuIndex++;
    }
    else if (key == Qt::Key_Up || key == Qt::Key_W) {
        if (menuIndex >= 2) menuIndex -= 2;
    }
    else if (key == Qt::Key_Down || key == Qt::Key_S) {
        if (menuIndex + 2 < maxIndex) menuIndex += 2;
    }
    else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (inCommandMenu) {
            emit commandSelected(menuIndex);
        } else if (inMoveMenu) {
            emit moveSelected(menuIndex);
        } else if (inPokemonMenu) {
            emit pokemonSelected(menuIndex);
        }
        return;
    }
    
    updateCursor();
}

void Menu_BT::updateCursor()
{
    if (!cursorSprite) return;
    
    QGraphicsTextItem *target = nullptr;
    
    if (inMoveMenu && !moveMenuOptions.isEmpty() && menuIndex < moveMenuOptions.size()) {
        target = moveMenuOptions[menuIndex];
    } else if (inPokemonMenu && !pokemonMenuOptions.isEmpty() && menuIndex < pokemonMenuOptions.size()) {
        target = pokemonMenuOptions[menuIndex];
    } else if (inCommandMenu && !commandMenuOptions.isEmpty() && menuIndex < commandMenuOptions.size()) {
        target = commandMenuOptions[menuIndex];
    }
    
    if (target) {
        cursorSprite->setPos(target->pos().x() - 28, target->pos().y() - 2);
        cursorSprite->setVisible(true);
    } else {
        cursorSprite->setVisible(false);
    }
}

void Menu_BT::destroyCommandMenu()
{
    if (commandMenuRect) {
        scene->removeItem(commandMenuRect);
        delete commandMenuRect;
        commandMenuRect = nullptr;
    }
    
    for (QGraphicsTextItem *t : commandMenuOptions) {
        if (t) {
            scene->removeItem(t);
            delete t;
        }
    }
    commandMenuOptions.clear();
    inCommandMenu = false;
}

void Menu_BT::destroyMoveMenu()
{
    if (moveMenuRect) {
        scene->removeItem(moveMenuRect);
        delete moveMenuRect;
        moveMenuRect = nullptr;
    }
    
    for (QGraphicsTextItem *t : moveMenuOptions) {
        if (t) {
            scene->removeItem(t);
            delete t;
        }
    }
    moveMenuOptions.clear();
    inMoveMenu = false;
}

void Menu_BT::destroyPokemonMenu()
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
    inPokemonMenu = false;
}


#ifndef MENU_OW_H
#define MENU_OW_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QKeyEvent>
#include <QVector>
#include "../Battle/Battle_logic/Player.h"

class Menu_OW : public QObject
{
    Q_OBJECT

public:
    Menu_OW(QGraphicsScene *scene, QGraphicsView *view, QObject *parent = nullptr);
    ~Menu_OW();

    void showMenu();
    void hideMenu();
    void handleKey(QKeyEvent *event);
    bool isInMenu() const { return inMenu || inPokemonMenu || inPokemonListMenu; }
    
    void setPlayer(Player *player) { gamePlayer = player; }

signals:
    void menuClosed();

private:
    QGraphicsScene *scene;
    QGraphicsView *view;
    Player *gamePlayer = nullptr;

    bool inMenu = false;
    bool inPokemonMenu = false;
    bool inPokemonListMenu = false;
    int menuIndex = 0;
    int selectedSlotIndex = -1;

    QGraphicsRectItem *menuRect = nullptr;
    QGraphicsRectItem *pokemonMenuRect = nullptr;
    QGraphicsRectItem *pokemonListMenuRect = nullptr;
    QVector<QGraphicsTextItem*> menuOptions;
    QVector<QGraphicsTextItem*> pokemonMenuOptions;
    QVector<QGraphicsTextItem*> pokemonListMenuOptions;
    QGraphicsPixmapItem *cursorSprite = nullptr;

    void menuSelected(int index);
    void showPokemonMenu();
    void showPokemonListMenu(int slotIndex);
    void hideSubMenu();
    void updateCursor();
    void pokemonSelected(int index);
    void pokemonListSelected(int index);
    void swapOrReplacePokemon(int slotIndex, int pokemonIndex);
};

#endif // MENU_OW_H


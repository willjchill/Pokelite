#ifndef MENU_BT_H
#define MENU_BT_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QKeyEvent>
#include <QVector>
#include <QString>

class Menu_BT : public QObject
{
    Q_OBJECT

public:
    Menu_BT(QGraphicsScene *scene, QObject *parent = nullptr);
    ~Menu_BT();

    void showDialogue(const QString &text);
    void showCommandMenu();
    void showMoveMenu(const QVector<QString> &moves);
    void showPokemonMenu(const QVector<QString> &pokemon);
    void hideAllMenus();
    
    void handleKey(QKeyEvent *event);
    bool isInMenu() const { return inCommandMenu || inMoveMenu || inPokemonMenu; }
    
    int getSelectedIndex() const { return menuIndex; }

signals:
    void commandSelected(int index);
    void moveSelected(int index);
    void pokemonSelected(int index);

private:
    QGraphicsScene *scene;
    
    bool inCommandMenu = false;
    bool inMoveMenu = false;
    bool inPokemonMenu = false;
    int menuIndex = 0;
    
    QGraphicsRectItem *commandMenuRect = nullptr;
    QGraphicsRectItem *moveMenuRect = nullptr;
    QGraphicsRectItem *pokemonMenuRect = nullptr;
    QGraphicsTextItem *dialogueText = nullptr;
    QVector<QGraphicsTextItem*> commandMenuOptions;
    QVector<QGraphicsTextItem*> moveMenuOptions;
    QVector<QGraphicsTextItem*> pokemonMenuOptions;
    QGraphicsPixmapItem *cursorSprite = nullptr;
    
    void updateCursor();
    void destroyCommandMenu();
    void destroyMoveMenu();
    void destroyPokemonMenu();
};

#endif // MENU_BT_H


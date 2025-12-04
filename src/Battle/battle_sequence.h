#ifndef BATTLE_SEQUENCE_H
#define BATTLE_SEQUENCE_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QKeyEvent>
#include <QTimer>
#include <QVector>
#include <functional>
#include <QObject>
#include "battle_system.h"
#include "Battle_logic/Player.h"
#include "battle_animations.h"

class BattleSequence : public QObject
{
    Q_OBJECT

public:
    BattleSequence(QGraphicsScene *scene, QGraphicsView *view);
    ~BattleSequence();

    // Battle initialization
    void startBattle(Player* player, Player* enemy, BattleSystem* battleSystem);
    void closeBattle();

    // UI setup
    void setupBattleUI();
    void updateBattleUI();

    // Input handling
    void handleBattleKey(QKeyEvent *event);
    void updateBattleCursor();

    // Battle flow
    void playerSelectedOption(int index);
    void playerChoseMove(int moveIndex);
    void enemyTurn();
    void showBagMenu();
    void showPokemonMenu();
    void playerSelectedBagItem(int index);
    void playerSelectedPokemon(int index);

    // Animations
    void battleZoomReveal();
    void animateBattleEntrances();
    void slideInCommandMenu();
    void slideOutCommandMenu(std::function<void()> onFinished = nullptr);
    void fadeInBattleScreen();
    void fadeOutBattleScreen(std::function<void()> onFinished = nullptr);

    // Getters
    bool isInBattle() const { return inBattle; }
    bool isInMenu() const { return inBattleMenu || inBagMenu || inPokemonMenu; }
    QGraphicsScene* getScene() const { return battleScene; }

    // Text display
    void setBattleText(const QString &text);
    void startTextAnimation();

signals:
    void battleEnded();

    friend class BattleAnimations;

private:
    QGraphicsScene *battleScene;
    QGraphicsView *view;
    BattleAnimations animations;

    // Battle state
    bool inBattle = false;
    BattleSystem* battleSystem = nullptr;
    Player* gamePlayer = nullptr;
    Player* enemyPlayer = nullptr;

    // Battle UI elements
    QGraphicsPixmapItem *battleTrainerItem = nullptr;
    QGraphicsPixmapItem *battlePlayerPokemonItem = nullptr;
    QGraphicsPixmapItem *battleEnemyItem = nullptr;

    // HP bars
    QGraphicsRectItem *enemyHpBack = nullptr;
    QGraphicsRectItem *enemyHpFill = nullptr;
    QGraphicsRectItem *playerHpBack = nullptr;
    QGraphicsRectItem *playerHpFill = nullptr;
    QGraphicsPixmapItem *enemyHpBackSprite = nullptr;
    QGraphicsPixmapItem *playerHpBackSprite = nullptr;
    QGraphicsRectItem *enemyHpMask = nullptr;
    QGraphicsRectItem *playerHpMask = nullptr;

    // Text elements
    QGraphicsTextItem *enemyPokemonNameText = nullptr;
    QGraphicsTextItem *playerPokemonNameText = nullptr;
    QGraphicsRectItem *battleTextBoxRect = nullptr;
    QGraphicsTextItem *battleTextItem = nullptr;
    QGraphicsPixmapItem *dialogueBoxSprite = nullptr;
    QString fullBattleText;
    int battleTextIndex = 0;
    QTimer battleTextTimer;

    // Menu system
    int battleMenuIndex = 0;
    bool inBattleMenu = false;
    bool inBagMenu = false;
    bool inPokemonMenu = false;
    QGraphicsRectItem *battleMenuRect = nullptr;
    QGraphicsPixmapItem *commandBoxSprite = nullptr;
    QGraphicsPixmapItem *battleCursorSprite = nullptr;
    QVector<QGraphicsTextItem*> battleMenuOptions;

    // Move menu
    QGraphicsRectItem *moveMenuRect = nullptr;
    QVector<QGraphicsTextItem*> moveMenuOptions;

    // Bag menu
    QGraphicsRectItem *bagMenuRect = nullptr;
    QVector<QGraphicsTextItem*> bagMenuOptions;
    QVector<int> bagMenuItemIndices; // Maps menu index to actual bag item index

    // Pokemon menu
    QGraphicsRectItem *pokemonMenuRect = nullptr;
    QVector<QGraphicsTextItem*> pokemonMenuOptions;

    // Animation effects
    QGraphicsOpacityEffect *fadeEffect = nullptr;
    QPropertyAnimation *fadeAnim = nullptr;
    QPropertyAnimation *cmdSlideAnim = nullptr;
    QPropertyAnimation *dialogueSlideAnim = nullptr;
    QGraphicsEllipseItem *battleCircleMask = nullptr;
    QVariantAnimation *battleCircleAnim = nullptr;

    // Helper functions
    void setHpColor(QGraphicsRectItem *hpBar, float hpPercent);
    void destroyMoveMenu();
    void destroyBagMenu();
    void destroyPokemonMenu();
    QString capitalizeFirst(const QString& str) const;
    void attemptCatchPokemon(int itemIndex);
    bool checkAndAutoSwitchPokemon(); // Returns true if switched, false if no Pokemon available

    friend class BattleAnimations;
};

#endif // BATTLE_SEQUENCE_H


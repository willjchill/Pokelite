#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QShowEvent>
#include <QTimer>
#include <QPixmap>
#include <QImage>
#include <vector>
#include <map>
#include <QColor>
#include <QRandomGenerator>
#include <QVector>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include "gamepadthread.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    // ---------- Overworld ----------
    QGraphicsScene *scene = nullptr;
    QGraphicsView  *view  = nullptr;

    QGraphicsPixmapItem *background = nullptr;
    QGraphicsPixmapItem *player     = nullptr;

    float speed    = 4.0f;
    bool  isMoving = false;

    void clampPlayer();
    bool isSolidPixel(int x, int y);
    bool isSlowPixel (int x, int y);
    bool isGrassPixel(int x, int y);

    std::map<QString, std::vector<QPixmap>> animations;
    QString currentDirection = "front";
    int     frameIndex       = 1;
    QTimer  animationTimer;

    void loadAnimations();

    QImage collisionMask;
    QImage tallGrassMask;

    // ---------- Battle ----------
    bool inBattle = false;

    QGraphicsScene *battleScene      = nullptr;
    QGraphicsPixmapItem *battleTrainerItem = nullptr;
    QGraphicsPixmapItem *battleEnemyItem   = nullptr;

    // Battle input
    int battleMenuIndex = 0;
    bool inBattleMenu = false;
    QGraphicsTextItem *battleCursor = nullptr;

    void updateBattleCursor();
    void handleBattleKey(QKeyEvent *event);


    // Fade overlay
    QGraphicsRectItem *battleFadeRect = nullptr;
    QTimer battleFadeTimer;

    // HP bars
    QGraphicsRectItem *enemyHpBack  = nullptr;
    QGraphicsRectItem *enemyHpFill  = nullptr;
    QGraphicsRectItem *playerHpBack = nullptr;
    QGraphicsRectItem *playerHpFill = nullptr;

    // Text box + typewriter
    QGraphicsRectItem *battleTextBoxRect = nullptr;
    QGraphicsTextItem *battleTextItem    = nullptr;
    QString fullBattleText;
    int battleTextIndex = 0;
    QTimer battleTextTimer;

    // Command menu
    QGraphicsRectItem *battleMenuRect = nullptr;
    QVector<QGraphicsTextItem*> battleMenuOptions;

    QGraphicsPixmapItem *enemyHpBackSprite = nullptr;
    QGraphicsPixmapItem *playerHpBackSprite = nullptr;
    QGraphicsPixmapItem *dialogueBoxSprite = nullptr;
    QGraphicsPixmapItem *commandBoxSprite = nullptr;
    QGraphicsPixmapItem *battleCursorSprite = nullptr;

    QGraphicsRectItem *enemyHpMask = nullptr;
    QGraphicsRectItem *playerHpMask = nullptr;
    void setHpColor(QGraphicsRectItem *hpBar, float hpPercent);


    QGraphicsOpacityEffect *fadeEffect = nullptr;
    QPropertyAnimation *fadeAnim = nullptr;

    QPropertyAnimation *cmdSlideAnim = nullptr;
    QPropertyAnimation *dialogueSlideAnim = nullptr;

    void fadeInBattleScreen();
    void fadeOutBattleScreen(std::function<void()> onFinished = nullptr);

    QGraphicsEllipseItem *battleCircleMask = nullptr;
    QVariantAnimation *battleCircleAnim = nullptr;

    void battleZoomReveal();
    void slideInCommandMenu();
    void slideOutCommandMenu(std::function<void()> onFinished = nullptr);

    void animateMenuSelection(int index);
    void animateCommandSelection(int index);


    // Encounter trigger
    void tryWildEncounter();
    void setupBattleUI();
    void animateBattleEntrances();

    // Gamepad support
    GamepadThread *gamepadThread;
    void handleGamepadInput(int type, int code, int value);
    void simulateKeyPress(Qt::Key key);
    void simulateKeyRelease(Qt::Key key);

};

#endif // MAINWINDOW_H

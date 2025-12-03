#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QShowEvent>
#include "gamepadthread.h"
#include "battle/battle_system.h"
#include "game_logic/Player.h"
#include "game_logic/Pokemon.h"
#include "game_logic/PokemonData.h"

// Forward declarations
class Overworld;
class BattleSequence;

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

private slots:
    void onWildEncounterTriggered();
    void onBattleEnded();

private:
    // ============================================================
    // CORE QT WINDOW MANAGEMENT
    // ============================================================
    QGraphicsScene *scene = nullptr;
    QGraphicsView  *view  = nullptr;

    // ============================================================
    // GAME SYSTEMS
    // ============================================================
    Overworld *overworld = nullptr;
    BattleSequence *battleSequence = nullptr;

    // ============================================================
    // GAME STATE
    // ============================================================
    bool inBattle = false;
    Player* gamePlayer = nullptr;
    Player* enemyPlayer = nullptr;
    BattleSystem* battleSystem = nullptr;

    // ============================================================
    // GAMEPAD SUPPORT
    // ============================================================
    GamepadThread *gamepadThread = nullptr;
    void handleGamepadInput(int type, int code, int value);
    void simulateKeyPress(Qt::Key key);
    void simulateKeyRelease(Qt::Key key);

    // ============================================================
    // HELPER FUNCTIONS
    // ============================================================
    void initializePlayer();
    void startWildEncounter();
};

#endif // MAINWINDOW_H

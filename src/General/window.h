#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QShowEvent>
#include "gamepad.h"
#include "../Battle/battle_system.h"
#include "../Battle/Battle_logic/Player.h"
#include "../Battle/Battle_logic/Pokemon.h"
#include "../Battle/Battle_logic/PokemonData.h"

// Forward declarations
class Overworld;
class BattleSequence;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    Window(QWidget *parent = nullptr);
    ~Window();

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
    Gamepad *gamepadThread = nullptr;
    void handleGamepadInput(int type, int code, int value);
    void simulateKeyPress(Qt::Key key);
    void simulateKeyRelease(Qt::Key key);

    // ============================================================
    // HELPER FUNCTIONS
    // ============================================================
    void initializePlayer();
    void startWildEncounter();
};

#endif // WINDOW_H

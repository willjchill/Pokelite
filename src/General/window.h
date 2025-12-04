#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QShowEvent>
#include "gamepad.h"
#include "uart_comm.h"
#include "../Battle/BattleState_BT.h"
#include "../Battle/Battle_logic/Player.h"
#include "../Battle/Battle_logic/Pokemon.h"
#include "../Battle/Battle_logic/PokemonData.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QTimer>

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
    void onPvpBattleRequested();
    void onUartPacketReceived(const BattlePacket& packet);
    void onPlayerFound();

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
    // UART COMMUNICATION
    // ============================================================
    UartComm *uartComm = nullptr;
    void showFindingPlayerPopup();
    void hideFindingPlayerPopup();
    void startPvpBattle();

    // ============================================================
    // HELPER FUNCTIONS
    // ============================================================
    void initializePlayer();
    void startWildEncounter();
    
    // ============================================================
    // PVP BATTLE UI
    // ============================================================
    bool findingPlayer = false;
    QGraphicsRectItem *findingPlayerRect = nullptr;
    QGraphicsTextItem *findingPlayerText = nullptr;
};

#endif // WINDOW_H

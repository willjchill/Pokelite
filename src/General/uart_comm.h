#ifndef UART_COMM_H
#define UART_COMM_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QString>

// Packet types for PvP battle communication
enum class PacketType {
    FINDING_PLAYER,      // Sent when player presses Q/SELECT to find opponent
    READY_BATTLE,        // Sent when both players are ready to start battle
    TURN,                // Sent when a player completes their turn
    BATTLE_END,          // Sent when battle ends
    POKEMON_DATA,        // Sent to sync Pokemon data at battle start
    INVALID
};

// Structure for battle packets
struct BattlePacket {
    PacketType type;
    QString data;  // Additional data (move index, Pokemon info, etc.)
    
    BattlePacket() : type(PacketType::INVALID) {}
    BattlePacket(PacketType t, const QString& d = "") : type(t), data(d) {}
    
    // Serialize to string for transmission
    QString serialize() const;
    
    // Deserialize from string
    static BattlePacket deserialize(const QString& str);
};

class UartComm : public QObject
{
    Q_OBJECT

public:
    explicit UartComm(QObject *parent = nullptr);
    ~UartComm();
    
    // Initialize UART connection
    bool initialize(const QString& portName = "/dev/ttyS1", qint32 baudRate = 115200);
    
    // Close UART connection
    void close();
    
    // Send packet
    bool sendPacket(const BattlePacket& packet);
    
    // Check if connected
    bool isConnected() const { return serialPort && serialPort->isOpen(); }
    
    // Check if currently finding player
    bool isFindingPlayer() const { return findingPlayer; }
    
    // Start/stop finding player
    void startFindingPlayer();
    void stopFindingPlayer();

signals:
    void packetReceived(const BattlePacket& packet);
    void connectionStatusChanged(bool connected);
    void playerFound();  // Emitted when READY_BATTLE is received

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void sendFindingPlayerPacket();

private:
    QSerialPort *serialPort;
    QTimer *findingPlayerTimer;
    bool findingPlayer;
    QString receiveBuffer;
    
    // Parse incoming data
    void parseReceivedData();
};

#endif // UART_COMM_H


#include "uart_comm.h"
#include <QDebug>
#include <QByteArray>

// Serialize packet to string format: "TYPE:data\n"
QString BattlePacket::serialize() const
{
    QString typeStr;
    switch (type) {
        case PacketType::FINDING_PLAYER:
            typeStr = "FINDING_PLAYER";
            break;
        case PacketType::READY_BATTLE:
            typeStr = "READY_BATTLE";
            break;
        case PacketType::TURN:
            typeStr = "TURN";
            break;
        case PacketType::BATTLE_END:
            typeStr = "BATTLE_END";
            break;
        case PacketType::POKEMON_DATA:
            typeStr = "POKEMON_DATA";
            break;
        default:
            typeStr = "INVALID";
            break;
    }
    
    if (data.isEmpty()) {
        return typeStr + "\n";
    } else {
        return typeStr + ":" + data + "\n";
    }
}

// Deserialize string to packet
BattlePacket BattlePacket::deserialize(const QString& str)
{
    BattlePacket packet;
    
    QString trimmed = str.trimmed();
    if (trimmed.isEmpty()) {
        return packet;
    }
    
    int colonIndex = trimmed.indexOf(':');
    QString typeStr;
    QString dataStr;
    
    if (colonIndex >= 0) {
        typeStr = trimmed.left(colonIndex);
        dataStr = trimmed.mid(colonIndex + 1);
    } else {
        typeStr = trimmed;
    }
    
    // Parse packet type
    if (typeStr == "FINDING_PLAYER") {
        packet.type = PacketType::FINDING_PLAYER;
    } else if (typeStr == "READY_BATTLE") {
        packet.type = PacketType::READY_BATTLE;
    } else if (typeStr == "TURN") {
        packet.type = PacketType::TURN;
    } else if (typeStr == "BATTLE_END") {
        packet.type = PacketType::BATTLE_END;
    } else if (typeStr == "POKEMON_DATA") {
        packet.type = PacketType::POKEMON_DATA;
    } else {
        packet.type = PacketType::INVALID;
    }
    
    packet.data = dataStr;
    return packet;
}

UartComm::UartComm(QObject *parent)
    : QObject(parent), serialPort(nullptr), findingPlayerTimer(nullptr),
      findingPlayer(false)
{
    serialPort = new QSerialPort(this);
    findingPlayerTimer = new QTimer(this);
    
    connect(serialPort, &QSerialPort::readyRead, this, &UartComm::handleReadyRead);
    connect(serialPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::error),
            this, &UartComm::handleError);
    connect(findingPlayerTimer, &QTimer::timeout, this, &UartComm::sendFindingPlayerPacket);
}

UartComm::~UartComm()
{
    close();
}

bool UartComm::initialize(const QString& portName, qint32 baudRate)
{
    if (serialPort->isOpen()) {
        serialPort->close();
    }
    
    serialPort->setPortName(portName);
    serialPort->setBaudRate(baudRate);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    
    if (!serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "Failed to open UART port:" << portName << serialPort->errorString();
        emit connectionStatusChanged(false);
        return false;
    }
    
    qDebug() << "UART port opened successfully:" << portName;
    emit connectionStatusChanged(true);
    return true;
}

void UartComm::close()
{
    stopFindingPlayer();
    
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
        emit connectionStatusChanged(false);
    }
}

bool UartComm::sendPacket(const BattlePacket& packet)
{
    if (!serialPort || !serialPort->isOpen()) {
        qDebug() << "Cannot send packet: UART not open";
        return false;
    }
    
    QString packetStr = packet.serialize();
    QByteArray data = packetStr.toUtf8();
    
    qint64 bytesWritten = serialPort->write(data);
    if (bytesWritten == -1) {
        qDebug() << "Error writing to UART:" << serialPort->errorString();
        return false;
    }
    
    serialPort->flush();
    qDebug() << "Sent packet:" << packetStr.trimmed();
    return true;
}

void UartComm::startFindingPlayer()
{
    if (findingPlayer) return;
    
    findingPlayer = true;
    // Send initial packet immediately
    sendFindingPlayerPacket();
    // Then send every 500ms
    findingPlayerTimer->start(500);
}

void UartComm::stopFindingPlayer()
{
    findingPlayer = false;
    findingPlayerTimer->stop();
}

void UartComm::sendFindingPlayerPacket()
{
    if (findingPlayer && isConnected()) {
        BattlePacket packet(PacketType::FINDING_PLAYER);
        sendPacket(packet);
    }
}

void UartComm::handleReadyRead()
{
    if (!serialPort) return;
    
    QByteArray data = serialPort->readAll();
    receiveBuffer += QString::fromUtf8(data);
    
    // Parse complete packets (ending with \n)
    parseReceivedData();
}

void UartComm::parseReceivedData()
{
    while (receiveBuffer.contains('\n')) {
        int newlineIndex = receiveBuffer.indexOf('\n');
        QString line = receiveBuffer.left(newlineIndex);
        receiveBuffer = receiveBuffer.mid(newlineIndex + 1);
        
        if (!line.isEmpty()) {
            BattlePacket packet = BattlePacket::deserialize(line);
            
            if (packet.type != PacketType::INVALID) {
                qDebug() << "Received packet:" << line;
                
                // Handle READY_BATTLE specially
                if (packet.type == PacketType::READY_BATTLE) {
                    stopFindingPlayer();
                    emit playerFound();
                }
                
                // If we're finding a player and receive FINDING_PLAYER, send READY_BATTLE
                if (packet.type == PacketType::FINDING_PLAYER && findingPlayer) {
                    BattlePacket readyPacket(PacketType::READY_BATTLE);
                    sendPacket(readyPacket);
                }
                
                emit packetReceived(packet);
            }
        }
    }
}

void UartComm::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        qDebug() << "UART resource error, closing connection";
        close();
    } else if (error != QSerialPort::NoError) {
        qDebug() << "UART error:" << serialPort->errorString();
    }
}


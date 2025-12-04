#include "uart_comm.h"
#include <QDebug>
#include <QByteArray>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

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
    : QObject(parent), uartFd(-1), readNotifier(nullptr), findingPlayerTimer(nullptr),
      findingPlayer(false)
{
    findingPlayerTimer = new QTimer(this);
    connect(findingPlayerTimer, &QTimer::timeout, this, &UartComm::sendFindingPlayerPacket);
}

UartComm::~UartComm()
{
    close();
}

bool UartComm::initialize(const QString& portName, qint32 baudRate)
{
    // Close existing connection if any
    close();
    
    // Open UART device
    QByteArray portBytes = portName.toLocal8Bit();
    uartFd = ::open(portBytes.constData(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    if (uartFd < 0) {
        qDebug() << "Failed to open UART port:" << portName << "Error:" << strerror(errno);
        emit connectionStatusChanged(false);
        return false;
    }
    
    // Configure serial port using termios
    struct termios tty;
    if (tcgetattr(uartFd, &tty) != 0) {
        qDebug() << "Failed to get UART attributes:" << strerror(errno);
        ::close(uartFd);
        uartFd = -1;
        emit connectionStatusChanged(false);
        return false;
    }
    
    // Set baud rate
    speed_t speed;
    switch (baudRate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        default: speed = B115200; break;
    }
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    // 8N1 configuration
    tty.c_cflag &= ~PARENB;  // No parity
    tty.c_cflag &= ~CSTOPB;  // 1 stop bit
    tty.c_cflag &= ~CSIZE;   // Clear size bits
    tty.c_cflag |= CS8;      // 8 data bits
    tty.c_cflag &= ~CRTSCTS; // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines
    
    // Disable canonical mode (raw input)
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // Disable software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    
    // Raw output
    tty.c_oflag &= ~OPOST;
    
    // Set read timeout (0.1 seconds)
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    
    // Apply settings
    if (tcsetattr(uartFd, TCSANOW, &tty) != 0) {
        qDebug() << "Failed to set UART attributes:" << strerror(errno);
        ::close(uartFd);
        uartFd = -1;
        emit connectionStatusChanged(false);
        return false;
    }
    
    // Create socket notifier for async reading
    readNotifier = new QSocketNotifier(uartFd, QSocketNotifier::Read, this);
    connect(readNotifier, &QSocketNotifier::activated, this, &UartComm::handleReadyRead);
    readNotifier->setEnabled(true);
    
    qDebug() << "UART port opened successfully:" << portName;
    emit connectionStatusChanged(true);
    return true;
}

void UartComm::close()
{
    stopFindingPlayer();
    
    if (readNotifier) {
        readNotifier->setEnabled(false);
        delete readNotifier;
        readNotifier = nullptr;
    }
    
    if (uartFd >= 0) {
        ::close(uartFd);
        uartFd = -1;
        emit connectionStatusChanged(false);
    }
}

bool UartComm::sendPacket(const BattlePacket& packet)
{
    if (uartFd < 0) {
        qDebug() << "Cannot send packet: UART not open";
        return false;
    }
    
    QString packetStr = packet.serialize();
    QByteArray data = packetStr.toUtf8();
    
    ssize_t bytesWritten = ::write(uartFd, data.constData(), data.size());
    if (bytesWritten < 0) {
        qDebug() << "Error writing to UART:" << strerror(errno);
        return false;
    }
    
    // Ensure data is written
    tcdrain(uartFd);
    
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
    if (uartFd < 0) return;
    
    char buffer[256];
    ssize_t bytesRead = ::read(uartFd, buffer, sizeof(buffer) - 1);
    
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        receiveBuffer += QString::fromUtf8(buffer, bytesRead);
        
        // Parse complete packets (ending with \n)
        parseReceivedData();
    } else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        qDebug() << "Error reading from UART:" << strerror(errno);
        close();
    }
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



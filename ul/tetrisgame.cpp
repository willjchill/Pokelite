#include "tetrisgame.h"
#include <QMessageBox>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <dirent.h>
#include <cstring>

// Linux input event codes for Xbox controller
#define BTN_A 304
#define BTN_B 305
#define BTN_X 307
#define BTN_Y 308
#define ABS_X 0
#define ABS_HAT0Y 17  // D-pad vertical

TetrisGame::TetrisGame(QWidget *parent) 
    : QWidget(parent), score(0), gameOver(false), fallSpeed(500), lastAxisX(0) {
    
    setFixedSize(BOARD_WIDTH * BLOCK_SIZE + 200, BOARD_HEIGHT * BLOCK_SIZE + 40);
    
    srand(time(nullptr));
    
    initBoard();
    initPieces();
    spawnNewPiece();
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TetrisGame::gameLoop);
    timer->start(fallSpeed);
    
    // Setup gamepad thread
    gamepadThread = new GamepadThread(this);
    connect(gamepadThread, &GamepadThread::inputReceived, 
            this, &TetrisGame::handleGamepadInput);
    gamepadThread->start();
    
    setFocusPolicy(Qt::StrongFocus);
}

TetrisGame::~TetrisGame() {
    delete timer;
    if (gamepadThread) {
        gamepadThread->stop();
        gamepadThread->wait();
        delete gamepadThread;
    }
}

void TetrisGame::initBoard() {
    board.resize(BOARD_HEIGHT, std::vector<int>(BOARD_WIDTH, 0));
}

void TetrisGame::initPieces() {
    // I piece
    pieceShapes.push_back({
        {1, 1, 1, 1}
    });
    
    // O piece
    pieceShapes.push_back({
        {1, 1},
        {1, 1}
    });
    
    // T piece
    pieceShapes.push_back({
        {0, 1, 0},
        {1, 1, 1}
    });
    
    // S piece
    pieceShapes.push_back({
        {0, 1, 1},
        {1, 1, 0}
    });
    
    // Z piece
    pieceShapes.push_back({
        {1, 1, 0},
        {0, 1, 1}
    });
    
    // J piece
    pieceShapes.push_back({
        {1, 0, 0},
        {1, 1, 1}
    });
    
    // L piece
    pieceShapes.push_back({
        {0, 0, 1},
        {1, 1, 1}
    });
}

TetrisGame::Piece TetrisGame::createRandomPiece() {
    Piece piece;
    int idx = rand() % pieceShapes.size();
    piece.shape = pieceShapes[idx];
    piece.x = BOARD_WIDTH / 2 - piece.shape[0].size() / 2;
    piece.y = 0;
    piece.color = idx + 1;
    return piece;
}

void TetrisGame::spawnNewPiece() {
    currentPiece = createRandomPiece();
    if (!canMove(0, 0)) {
        gameOver = true;
        timer->stop();
    }
}

bool TetrisGame::canMove(int dx, int dy) {
    for (size_t i = 0; i < currentPiece.shape.size(); i++) {
        for (size_t j = 0; j < currentPiece.shape[i].size(); j++) {
            if (currentPiece.shape[i][j]) {
                int newX = currentPiece.x + j + dx;
                int newY = currentPiece.y + i + dy;
                
                if (newX < 0 || newX >= BOARD_WIDTH || newY >= BOARD_HEIGHT) {
                    return false;
                }
                
                if (newY >= 0 && board[newY][newX]) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool TetrisGame::canRotate() {
    std::vector<std::vector<int>> rotated(currentPiece.shape[0].size(),
                                          std::vector<int>(currentPiece.shape.size()));
    
    for (size_t i = 0; i < currentPiece.shape.size(); i++) {
        for (size_t j = 0; j < currentPiece.shape[i].size(); j++) {
            rotated[j][currentPiece.shape.size() - 1 - i] = currentPiece.shape[i][j];
        }
    }
    
    for (size_t i = 0; i < rotated.size(); i++) {
        for (size_t j = 0; j < rotated[i].size(); j++) {
            if (rotated[i][j]) {
                int newX = currentPiece.x + j;
                int newY = currentPiece.y + i;
                
                if (newX < 0 || newX >= BOARD_WIDTH || newY >= BOARD_HEIGHT) {
                    return false;
                }
                
                if (newY >= 0 && board[newY][newX]) {
                    return false;
                }
            }
        }
    }
    return true;
}

void TetrisGame::movePiece(int dx, int dy) {
    if (!gameOver && canMove(dx, dy)) {
        currentPiece.x += dx;
        currentPiece.y += dy;
        update();
    }
}

void TetrisGame::rotatePiece() {
    if (!gameOver && canRotate()) {
        std::vector<std::vector<int>> rotated(currentPiece.shape[0].size(),
                                              std::vector<int>(currentPiece.shape.size()));
        
        for (size_t i = 0; i < currentPiece.shape.size(); i++) {
            for (size_t j = 0; j < currentPiece.shape[i].size(); j++) {
                rotated[j][currentPiece.shape.size() - 1 - i] = currentPiece.shape[i][j];
            }
        }
        
        currentPiece.shape = rotated;
        update();
    }
}

void TetrisGame::lockPiece() {
    for (size_t i = 0; i < currentPiece.shape.size(); i++) {
        for (size_t j = 0; j < currentPiece.shape[i].size(); j++) {
            if (currentPiece.shape[i][j]) {
                int boardY = currentPiece.y + i;
                int boardX = currentPiece.x + j;
                if (boardY >= 0) {
                    board[boardY][boardX] = currentPiece.color;
                }
            }
        }
    }
    clearLines();
    spawnNewPiece();
}

void TetrisGame::clearLines() {
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
        bool fullLine = true;
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == 0) {
                fullLine = false;
                break;
            }
        }
        
        if (fullLine) {
            score += 100;
            board.erase(board.begin() + i);
            board.insert(board.begin(), std::vector<int>(BOARD_WIDTH, 0));
            i++;
        }
    }
}

void TetrisGame::gameLoop() {
    if (!gameOver) {
        if (canMove(0, 1)) {
            currentPiece.y++;
        } else {
            lockPiece();
        }
        update();
    }
}

void TetrisGame::handleGamepadInput(int type, int code, int value) {
    if (type == EV_KEY) {  // Button event
        if (value == 1) {  // Button pressed
            if (gameOver) {
                resetGame();
            } else {
                if (code == BTN_A || code == BTN_X) {
                    rotatePiece();
                } else if (code == BTN_B || code == BTN_Y) {
                    movePiece(0, 1);
                }
            }
        }
    } else if (type == EV_ABS) {  // Axis event
        if (code == ABS_X) {  // Left stick horizontal
            int threshold = 20000;
            if (value > threshold && lastAxisX <= threshold) {
                movePiece(1, 0);
            } else if (value < -threshold && lastAxisX >= -threshold) {
                movePiece(-1, 0);
            }
            lastAxisX = value;
        } else if (code == ABS_HAT0Y) {  // D-pad vertical
            if (value > 0) {  // D-pad down
                movePiece(0, 1);
            }
        }
    }
}

void TetrisGame::resetGame() {
    initBoard();
    score = 0;
    gameOver = false;
    spawnNewPiece();
    timer->start(fallSpeed);
    update();
}

void TetrisGame::keyPressEvent(QKeyEvent *event) {
    if (gameOver) {
        if (event->key() == Qt::Key_R) {
            resetGame();
        }
        return;
    }
    
    switch (event->key()) {
        case Qt::Key_Left:
            movePiece(-1, 0);
            break;
        case Qt::Key_Right:
            movePiece(1, 0);
            break;
        case Qt::Key_Down:
            movePiece(0, 1);
            break;
        case Qt::Key_Up:
        case Qt::Key_Space:
            rotatePiece();
            break;
    }
}

void TetrisGame::drawBlock(QPainter &painter, int x, int y, int color) {
    QColor colors[] = {
        QColor(0, 0, 0),       // 0: empty
        QColor(0, 255, 255),   // 1: I - cyan
        QColor(255, 255, 0),   // 2: O - yellow
        QColor(128, 0, 128),   // 3: T - purple
        QColor(0, 255, 0),     // 4: S - green
        QColor(255, 0, 0),     // 5: Z - red
        QColor(0, 0, 255),     // 6: J - blue
        QColor(255, 165, 0)    // 7: L - orange
    };
    
    painter.fillRect(x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, colors[color]);
    painter.setPen(Qt::black);
    painter.drawRect(x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);
}

void TetrisGame::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    
    // Draw board
    painter.fillRect(0, 0, BOARD_WIDTH * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE, Qt::black);
    
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j]) {
                drawBlock(painter, j, i, board[i][j]);
            }
        }
    }
    
    // Draw current piece
    for (size_t i = 0; i < currentPiece.shape.size(); i++) {
        for (size_t j = 0; j < currentPiece.shape[i].size(); j++) {
            if (currentPiece.shape[i][j]) {
                drawBlock(painter, currentPiece.x + j, currentPiece.y + i, currentPiece.color);
            }
        }
    }
    
    // Draw score
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 16));
    painter.drawText(BOARD_WIDTH * BLOCK_SIZE + 20, 30, QString("Score: %1").arg(score));
    
    // Draw controls
    painter.setFont(QFont("Arial", 10));
    painter.drawText(BOARD_WIDTH * BLOCK_SIZE + 20, 80, "Keyboard:");
    painter.drawText(BOARD_WIDTH * BLOCK_SIZE + 20, 100, "Arrows: Move");
    painter.drawText(BOARD_WIDTH * BLOCK_SIZE + 20, 120, "Space/Up: Rotate");
    painter.drawText(BOARD_WIDTH * BLOCK_SIZE + 20, 140, "R: Restart");
    
    painter.drawText(BOARD_WIDTH * BLOCK_SIZE + 20, 180, "Gamepad:");
    painter.drawText(BOARD_WIDTH * BLOCK_SIZE + 20, 200, "Left Stick: Move");
    painter.drawText(BOARD_WIDTH * BLOCK_SIZE + 20, 220, "D-Pad Down: Drop");
    painter.drawText(BOARD_WIDTH * BLOCK_SIZE + 20, 240, "A/X: Rotate");
    painter.drawText(BOARD_WIDTH * BLOCK_SIZE + 20, 260, "B/Y: Drop");
    
    if (gameOver) {
        painter.setFont(QFont("Arial", 20, QFont::Bold));
        painter.setPen(Qt::red);
        painter.drawText(rect(), Qt::AlignCenter, "GAME OVER\nPress R to Restart");
    }
}

// GamepadThread implementation
GamepadThread::GamepadThread(QObject *parent) 
    : QThread(parent), running(true) {
}

GamepadThread::~GamepadThread() {
    stop();
}

void GamepadThread::stop() {
    running = false;
}

int GamepadThread::findGamepad() {
    DIR *dir = opendir("/dev/input");
    if (!dir) return -1;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            char path[256];
            snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);
            
            int fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd >= 0) {
                char name[256] = "Unknown";
                ioctl(fd, EVIOCGNAME(sizeof(name)), name);
                
                // Look for Xbox controller or generic gamepad
                if (strstr(name, "Xbox") || strstr(name, "Controller") || 
                    strstr(name, "Gamepad") || strstr(name, "pad")) {
                    closedir(dir);
                    return fd;
                }
                close(fd);
            }
        }
    }
    closedir(dir);
    return -1;
}

void GamepadThread::run() {
    int fd = findGamepad();
    if (fd < 0) {
        return;  // No gamepad found
    }
    
    struct input_event ev;
    fd_set readfds;
    struct timeval tv;
    
    while (running) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms timeout
        
        int ret = select(fd + 1, &readfds, nullptr, nullptr, &tv);
        
        if (ret > 0 && FD_ISSET(fd, &readfds)) {
            ssize_t bytes = read(fd, &ev, sizeof(ev));
            if (bytes == sizeof(ev)) {
                if (ev.type == EV_KEY || ev.type == EV_ABS) {
                    emit inputReceived(ev.type, ev.code, ev.value);
                }
            }
        }
    }
    
    close(fd);
}

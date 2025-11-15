#ifndef TETRISGAME_H
#define TETRISGAME_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include <QThread>
#include <vector>
#include <atomic>

// Forward declaration for gamepad thread
class GamepadThread;

class TetrisGame : public QWidget {
    Q_OBJECT
    
public:
    explicit TetrisGame(QWidget *parent = nullptr);
    ~TetrisGame();
    
signals:
    void gamepadInput(int type, int code, int value);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    
private slots:
    void gameLoop();
    void handleGamepadInput(int type, int code, int value);
    
private:
    static const int BOARD_WIDTH = 10;
    static const int BOARD_HEIGHT = 20;
    static const int BLOCK_SIZE = 30;
    
    struct Piece {
        std::vector<std::vector<int>> shape;
        int x, y;
        int color;
    };
    
    void initBoard();
    void initPieces();
    Piece createRandomPiece();
    void spawnNewPiece();
    bool canMove(int dx, int dy);
    bool canRotate();
    void movePiece(int dx, int dy);
    void rotatePiece();
    void lockPiece();
    void clearLines();
    bool isGameOver();
    void resetGame();
    void drawBlock(QPainter &painter, int x, int y, int color);
    
    std::vector<std::vector<int>> board;
    std::vector<std::vector<std::vector<int>>> pieceShapes;
    Piece currentPiece;
    QTimer *timer;
    GamepadThread *gamepadThread;
    
    int score;
    bool gameOver;
    int fallSpeed;
    int lastAxisX;
};

// Gamepad reading thread
class GamepadThread : public QThread {
    Q_OBJECT
    
public:
    GamepadThread(QObject *parent = nullptr);
    ~GamepadThread();
    void stop();
    
signals:
    void inputReceived(int type, int code, int value);
    
protected:
    void run() override;
    
private:
    std::atomic<bool> running;
    int findGamepad();
};

#endif // TETRISGAME_H

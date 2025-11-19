#include "mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Window size
    setFixedSize(380, 639);

    // Scene
    scene = new QGraphicsScene(0, 0, 380, 639, this);

    // View
    view = new QGraphicsView(scene, this);
    view->setFixedSize(380, 639);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Background map
    background = scene->addPixmap(QPixmap(":/assets/background.png"));
    background->setZValue(0);

    // Load collision mask
    collisionMask.load(":/assets/collision.png");
    if (collisionMask.isNull())
        qDebug() << "ERROR: collision.png failed to load!";

    // Load animations
    loadAnimations();

    // Player setup
    currentDirection = "front";
    frameIndex = 1;
    isMoving = false;

    player = scene->addPixmap(animations[currentDirection][frameIndex]);
    player->setZValue(1);
    player->setPos(160, 300);

    // Animation timer
    connect(&animationTimer, &QTimer::timeout, this, [this]() {
        auto &frames = animations[currentDirection];
        frameIndex = (frameIndex + 1) % frames.size();
        player->setPixmap(frames[frameIndex]);
    });
}

MainWindow::~MainWindow() {}

void MainWindow::loadAnimations()
{
    animations["front"] = {
        QPixmap(":/assets/front1.png"),
        QPixmap(":/assets/front2.png"),
        QPixmap(":/assets/front3.png")
    };
    animations["back"] = {
        QPixmap(":/assets/back1.png"),
        QPixmap(":/assets/back2.png"),
        QPixmap(":/assets/back3.png")
    };
    animations["left"] = {
        QPixmap(":/assets/left1.png"),
        QPixmap(":/assets/left2.png"),
        QPixmap(":/assets/left3.png")
    };
    animations["right"] = {
        QPixmap(":/assets/right1.png"),
        QPixmap(":/assets/right2.png"),
        QPixmap(":/assets/right3.png")
    };
}

// ---------------------------------------------------------
// PIXEL COLLISION HELPERS
// ---------------------------------------------------------

// BLACK -> SOLID AREA , PLAYER CANNOT MOVE THRU
// BLUE -> SLOW DOWN AREA, CAN MOVE THRU W REDUCED SPEED

bool MainWindow::isSolidPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= collisionMask.width() ||
        y >= collisionMask.height())
        return true; // outside map = solid

    QColor pixel = collisionMask.pixelColor(x, y);

    // Black = solid wall
    return (pixel.red() < 30 &&
            pixel.green() < 30 &&
            pixel.blue() < 30);
}

bool MainWindow::isSlowPixel(int x, int y)
{
    if (x < 0 || y < 0 ||
        x >= collisionMask.width() ||
        y >= collisionMask.height())
        return false;

    QColor pixel = collisionMask.pixelColor(x, y);

    // Blue = slow zone
    return (pixel.blue() > 200 &&
            pixel.red() < 80 &&
            pixel.green() < 80);
}

// ---------------------------------------------------------
// MOVEMENT + ANIMATION
// ---------------------------------------------------------

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    float dx = 0;
    float dy = 0;

    switch (event->key())
    {
    case Qt::Key_W: dy = -1; currentDirection = "back";  break;
    case Qt::Key_S: dy = +1; currentDirection = "front"; break;
    case Qt::Key_A: dx = -1; currentDirection = "left";  break;
    case Qt::Key_D: dx = +1; currentDirection = "right"; break;
    default:
        return;
    }

    // Check pixel under feet
    QPointF oldPos = player->pos();
    QPointF tryPos(oldPos.x() + dx * speed, oldPos.y() + dy * speed);

    int px = tryPos.x() + player->boundingRect().width() / 2;
    int py = tryPos.y() + player->boundingRect().height() - 4;

    // SLOW ZONE DETECTED: reduce speed
    float finalSpeed = speed;
    if (isSlowPixel(px, py))
        finalSpeed = speed * 0.4f;

    // Recalculate tryPos with slow speed
    tryPos = QPointF(oldPos.x() + dx * finalSpeed,
                     oldPos.y() + dy * finalSpeed);

    // Check collision again
    px = tryPos.x() + player->boundingRect().width() / 2;
    py = tryPos.y() + player->boundingRect().height() - 4;

    // If solid → do not move
    if (!isSolidPixel(px, py))
        player->setPos(tryPos);

    // Start animation
    if (!isMoving && (dx != 0 || dy != 0))
    {
        isMoving = true;
        frameIndex = 1;
        animationTimer.start(120);
    }

    clampPlayer();
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_W ||
        event->key() == Qt::Key_A ||
        event->key() == Qt::Key_S ||
        event->key() == Qt::Key_D)
    {
        isMoving = false;
        animationTimer.stop();
        frameIndex = 1;  // idle frame
        player->setPixmap(animations[currentDirection][frameIndex]);
    }
}

// ---------------------------------------------------------

void MainWindow::clampPlayer()
{
    QRectF bounds = player->boundingRect();
    QPointF pos = player->pos();

    if (pos.x() < 0) pos.setX(0);
    if (pos.x() > 380 - bounds.width())
        pos.setX(380 - bounds.width());

    if (pos.y() < 0) pos.setY(0);
    if (pos.y() > 639 - bounds.height())
        pos.setY(639 - bounds.height());

    player->setPos(pos);
}

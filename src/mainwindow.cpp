#include "mainwindow.h"
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Window size
    setFixedSize(380, 639);

    // Create scene
    scene = new QGraphicsScene(0, 0, 380, 639, this);

    // Create view
    view = new QGraphicsView(scene, this);
    view->setFixedSize(380, 639);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // --- Background (from QRC)
    QPixmap bg(":/assets/background.png");
    background = scene->addPixmap(bg);
    background->setZValue(0);

    // --- Player sprite (from QRC)
    QPixmap p(":/assets/front.png");
    player = scene->addPixmap(p);
    player->setZValue(1);

    // Starting position
    player->setPos(160, 300);
}

MainWindow::~MainWindow()
{
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    float dx = 0;
    float dy = 0;

    switch (event->key())
    {
    case Qt::Key_W: dy = -speed; break;
    case Qt::Key_S: dy = +speed; break;
    case Qt::Key_A: dx = -speed; break;
    case Qt::Key_D: dx = +speed; break;
    }

    // Move player
    player->moveBy(dx, dy);

    // Keep player inside window
    clampPlayer();
}

void MainWindow::clampPlayer()
{
    QRectF bounds = player->boundingRect();
    QPointF pos = player->pos();

    // Left & right
    if (pos.x() < 0) pos.setX(0);
    if (pos.x() > 380 - bounds.width())
        pos.setX(380 - bounds.width());

    // Top & bottom
    if (pos.y() < 0) pos.setY(0);
    if (pos.y() > 639 - bounds.height())
        pos.setY(639 - bounds.height());

    player->setPos(pos);
}

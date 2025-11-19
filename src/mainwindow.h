#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QKeyEvent>
#include <QTimer>
#include <QPixmap>
#include <QImage>
#include <vector>
#include <map>
#include <QImage>
#include <QColor>


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    // Scene + View
    QGraphicsScene *scene;
    QGraphicsView *view;

    // Sprites
    QGraphicsPixmapItem *background;
    QGraphicsPixmapItem *player;

    // Movement
    float speed = 4.0f;
    bool isMoving = false;

    void clampPlayer();
    // Area where players cannot go or slows down when moving over - BLACK for solid , BLUE for slow down zone
    bool isSolidPixel(int x, int y);
    bool isSlowPixel(int x, int y);


    // Animation
    std::map<QString, std::vector<QPixmap>> animations;
    QString currentDirection = "front";
    int frameIndex = 1;
    QTimer animationTimer;

    void loadAnimations();


    // Collision mask
    QImage collisionMask;
};

#endif // MAINWINDOW_H

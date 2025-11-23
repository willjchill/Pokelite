#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QGraphicsScene *scene;
    QGraphicsView *view;

    QGraphicsPixmapItem *background;
    QGraphicsPixmapItem *player;

    float speed = 4.0f;

    void clampPlayer();
};

#endif // MAINWINDOW_H

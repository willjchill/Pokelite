#ifndef PLAYER_OW_H
#define PLAYER_OW_H

#include <QGraphicsObject>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QMap>
#include <QString>
#include <QVector>
#include <QPixmap>

class Player_OW : public QGraphicsObject
{
    Q_OBJECT

public:
    Player_OW(QGraphicsItem *parent = nullptr);
    ~Player_OW();

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // Movement
    void setDirection(const QString &direction);
    QString getCurrentDirection() const { return currentDirection; }
    void setPosition(const QPointF &pos);
    QPointF getPosition() const;

    // Animation
    void loadAnimations();
    void startAnimation();
    void stopAnimation();

signals:
    void positionChanged();

private slots:
    void updateAnimation();

private:
    QString currentDirection;
    int frameIndex;
    QTimer animationTimer;
    std::map<QString, std::vector<QPixmap>> animations;
    QGraphicsPixmapItem *spriteItem;
};

#endif // PLAYER_OW_H


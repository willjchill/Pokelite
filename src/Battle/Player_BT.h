#ifndef PLAYER_BT_H
#define PLAYER_BT_H

#include <QGraphicsPixmapItem>
#include <QString>

class Player_BT : public QGraphicsPixmapItem
{
public:
    Player_BT(QGraphicsItem *parent = nullptr);
    ~Player_BT();

    void setSprite(const QString &spritePath);
    void setPosition(const QPointF &pos);
};

#endif // PLAYER_BT_H


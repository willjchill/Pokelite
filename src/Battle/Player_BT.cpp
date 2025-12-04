#include "Player_BT.h"
#include <QDebug>

Player_BT::Player_BT(QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
{
}

Player_BT::~Player_BT()
{
}

void Player_BT::setSprite(const QString &spritePath)
{
    QPixmap sprite(spritePath);
    if (!sprite.isNull()) {
        setPixmap(sprite);
    }
}

void Player_BT::setPosition(const QPointF &pos)
{
    setPos(pos);
}


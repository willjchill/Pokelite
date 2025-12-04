#include "Pokemon_BT.h"
#include <QDebug>

Pokemon_BT::Pokemon_BT(QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
{
}

Pokemon_BT::~Pokemon_BT()
{
}

void Pokemon_BT::setSprite(const QString &spritePath, bool isFront)
{
    QPixmap sprite(spritePath);
    if (!sprite.isNull()) {
        setPixmap(sprite);
    }
}

void Pokemon_BT::setPosition(const QPointF &pos)
{
    setPos(pos);
}

void Pokemon_BT::setScale(float scale)
{
    QGraphicsPixmapItem::setScale(scale);
}


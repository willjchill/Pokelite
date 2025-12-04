#include "Player_OW.h"
#include <QPainter>
#include <QDebug>

Player_OW::Player_OW(QGraphicsItem *parent)
    : QGraphicsObject(parent), currentDirection("front"), frameIndex(1)
{
    spriteItem = new QGraphicsPixmapItem(this);
    loadAnimations();
    
    connect(&animationTimer, &QTimer::timeout, this, &Player_OW::updateAnimation);
}

Player_OW::~Player_OW()
{
}

QRectF Player_OW::boundingRect() const
{
    if (spriteItem && !spriteItem->pixmap().isNull()) {
        return spriteItem->boundingRect();
    }
    return QRectF(0, 0, 32, 32);
}

void Player_OW::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
    // Painting is handled by spriteItem
}

void Player_OW::loadAnimations()
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
    
    // Set initial sprite
    if (!animations[currentDirection].empty()) {
        spriteItem->setPixmap(animations[currentDirection][frameIndex]);
    }
}

void Player_OW::setDirection(const QString &direction)
{
    if (currentDirection != direction) {
        currentDirection = direction;
        frameIndex = 1;
        if (!animations[currentDirection].empty() && spriteItem) {
            spriteItem->setPixmap(animations[currentDirection][frameIndex]);
        }
    }
}

void Player_OW::setPosition(const QPointF &pos)
{
    setPos(pos);
    emit positionChanged();
}

QPointF Player_OW::getPosition() const
{
    return pos();
}

void Player_OW::startAnimation()
{
    if (!animationTimer.isActive()) {
        animationTimer.start(120);
    }
}

void Player_OW::stopAnimation()
{
    animationTimer.stop();
    frameIndex = 1;
    if (!animations[currentDirection].empty() && spriteItem) {
        spriteItem->setPixmap(animations[currentDirection][frameIndex]);
    }
}

void Player_OW::updateAnimation()
{
    auto &frames = animations[currentDirection];
    if (!frames.empty()) {
        frameIndex = (frameIndex + 1) % frames.size();
        if (spriteItem) {
            spriteItem->setPixmap(frames[frameIndex]);
        }
    }
}


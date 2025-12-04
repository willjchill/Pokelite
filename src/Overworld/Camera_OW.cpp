#include "Camera_OW.h"
#include <QTransform>
#include <QDebug>

Camera_OW::Camera_OW(QGraphicsView *view)
    : view(view)
{
    if (view) {
        view->setFixedSize(viewW, viewH);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setupZoom();
    }
}

Camera_OW::~Camera_OW()
{
}

void Camera_OW::setupZoom()
{
    if (view) {
        // Zoom in closer to the player
        view->setTransform(QTransform::fromScale(zoomFactor, zoomFactor));
    }
}

void Camera_OW::removeZoom()
{
    if (view) {
        // Reset to no zoom (1:1 scale)
        view->resetTransform();
    }
}

void Camera_OW::centerOnPlayer(QGraphicsItem *player)
{
    if (!player || !view || !view->scene()) return;
    
    updateCamera(player);
}

void Camera_OW::updateCamera(QGraphicsItem *player)
{
    if (!player || !view || !view->scene()) return;

    QRectF mapRect = view->scene()->sceneRect();
    
    qreal targetX = player->x() + player->boundingRect().width()/2;
    qreal targetY = player->y() + player->boundingRect().height()/2;

    // Account for zoom factor (zoom means effective view is smaller)
    qreal effectiveW = viewW / zoomFactor;
    qreal effectiveH = viewH / zoomFactor;
    
    qreal halfW = effectiveW / 2;
    qreal halfH = effectiveH / 2;

    qreal minCX = mapRect.left() + halfW;
    qreal maxCX = mapRect.right() - halfW;
    qreal minCY = mapRect.top() + halfH;
    qreal maxCY = mapRect.bottom() - halfH;

    if (targetX < minCX) targetX = minCX;
    if (targetX > maxCX) targetX = maxCX;

    if (targetY < minCY) targetY = minCY;
    if (targetY > maxCY) targetY = maxCY;

    view->centerOn(targetX, targetY);
}


#ifndef CAMERA_OW_H
#define CAMERA_OW_H

#include <QGraphicsView>
#include <QGraphicsItem>

class Camera_OW
{
public:
    Camera_OW(QGraphicsView *view);
    ~Camera_OW();

    void centerOnPlayer(QGraphicsItem *player);
    void updateCamera(QGraphicsItem *player);
    void setupZoom();
    void removeZoom();

private:
    QGraphicsView *view;
    const qreal viewW = 480;
    const qreal viewH = 272;
    const qreal zoomFactor = 2.0;  // Zoom level (2.0 = 2x zoom)
};

#endif // CAMERA_OW_H


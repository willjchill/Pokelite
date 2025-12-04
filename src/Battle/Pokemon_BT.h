#ifndef POKEMON_BT_H
#define POKEMON_BT_H

#include <QGraphicsPixmapItem>
#include <QString>

class Pokemon_BT : public QGraphicsPixmapItem
{
public:
    Pokemon_BT(QGraphicsItem *parent = nullptr);
    ~Pokemon_BT();

    void setSprite(const QString &spritePath, bool isFront = true);
    void setPosition(const QPointF &pos);
    void setScale(float scale);
};

#endif // POKEMON_BT_H


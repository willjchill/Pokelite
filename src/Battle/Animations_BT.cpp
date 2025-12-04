#include "Animations_BT.h"
#include "GUI_BT.h"

#include <QPainterPath>
#include <QVariantAnimation>
#include <QTimer>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QVector>

Animations_BT::Animations_BT(QObject *parent)
    : QObject(parent)
{
}

// ================================================
// 0. TRAINER THROW ANIMATION (5 frames)
// ================================================
void Animations_BT::animateTrainerThrow(BattleSequence* b, std::function<void()> onFinished)
{
    if (!b || !b->battleTrainerItem || !b->getScene()) {
        if (onFinished) onFinished();
        return;
    }

    QGraphicsScene *scene = b->getScene();

    // Load 5 throw frames: :/assets/battle/battle_player_1.png ... _5.png
    QVector<QPixmap> frames;
    for (int i = 1; i <= 5; ++i) {
        QString path = QString(":/assets/battle/battle_player%1.png").arg(i);
        QPixmap px(path);
        if (!px.isNull())
            frames.push_back(px);
    }

    if (frames.isEmpty()) {
        // If sprites missing, just continue flow
        if (onFinished) onFinished();
        return;
    }

    // Temp sprite for animation
    QGraphicsPixmapItem *throwSprite = scene->addPixmap(frames[0]);
    throwSprite->setScale(b->battleTrainerItem->scale());
    throwSprite->setZValue(b->battleTrainerItem->zValue() + 1);
    throwSprite->setPos(b->battleTrainerItem->pos());

    // Hide idle trainer during throw
    b->battleTrainerItem->setVisible(false);

    int frameIndex = 0;
    QTimer *timer = new QTimer(this);
    timer->setInterval(100); // 100 ms per frame

    QObject::connect(timer, &QTimer::timeout, this,
                     [=]() mutable
                     {
                         ++frameIndex;

                         if (frameIndex >= frames.size()) {
                             timer->stop();

                             scene->removeItem(throwSprite);
                             delete throwSprite;

                             // We keep trainer hidden; Pokémon will appear instead
                             if (onFinished) onFinished();

                             timer->deleteLater();
                             return;
                         }

                         throwSprite->setPixmap(frames[frameIndex]);
                     });

    timer->start();
}

// ================================================
// 1. BATTLE ZOOM REVEAL (circular)
// ================================================
void Animations_BT::battleZoomReveal(BattleSequence *b)
{
    if (!b || !b->getScene()) return;

    QGraphicsScene *scene = b->getScene();

    QGraphicsPathItem *mask = new QGraphicsPathItem();
    mask->setBrush(Qt::black);
    mask->setPen(Qt::NoPen);
    mask->setZValue(9999);
    scene->addItem(mask);

    QPainterPath full;
    full.addRect(0, 0, 480, 272);

    QVariantAnimation *anim = new QVariantAnimation();
    anim->setDuration(2000);
    anim->setStartValue(0);
    anim->setEndValue(1100);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(anim, &QVariantAnimation::valueChanged,
                     [=](const QVariant &v)
                     {
                         qreal r = v.toReal();
                         QPainterPath hole;
                         hole.addEllipse(240 - r, 136 - r, 2*r, 2*r);
                         QPainterPath reveal = full.subtracted(hole);
                         mask->setPath(reveal);
                     });

    QObject::connect(anim, &QVariantAnimation::finished,
                     [=]()
                     {
                         scene->removeItem(mask);
                         delete mask;
                     });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// ================================================
// 2. ENTRANCE ANIMATION (slide-in both sides)
// ================================================
void Animations_BT::animateBattleEntrances(BattleSequence *b)
{
    if (!b || !b->battleEnemyItem) return;

    QGraphicsScene *scene = b->getScene();

    // -----------------------------
    // CIRCLE REVEAL MASK (same as before)
    // -----------------------------
    QGraphicsPathItem *mask = new QGraphicsPathItem();
    mask->setBrush(Qt::black);
    mask->setPen(Qt::NoPen);
    mask->setZValue(9999);
    scene->addItem(mask);

    QPainterPath full;
    full.addRect(0, 0, 480, 272);

    QVariantAnimation *circle = new QVariantAnimation();
    circle->setDuration(900);
    circle->setStartValue(0.0);
    circle->setEndValue(450.0);
    circle->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(circle, &QVariantAnimation::valueChanged,
                     [=](const QVariant &v)
                     {
                         qreal r = v.toReal();
                         QPainterPath hole;
                         hole.addEllipse(240 - r, 136 - r, 2*r, 2*r);
                         QPainterPath maskPath = full.subtracted(hole);
                         mask->setPath(maskPath);
                     });

    QObject::connect(circle, &QVariantAnimation::finished,
                     [=]()
                     {
                         scene->removeItem(mask);
                         delete mask;
                     });

    circle->start(QAbstractAnimation::DeleteWhenStopped);

    // -----------------------------
    // SLIDE-IN: PLAYER (TRAINER) + ENEMY
    // -----------------------------

    // 👉 ALWAYS prefer the trainer sprite on the player side
    QGraphicsPixmapItem *playerSprite =
        (b->battleTrainerItem) ? b->battleTrainerItem
                               : b->battlePlayerPokemonItem;

    QPointF playerFinal = playerSprite
                              ? playerSprite->pos()
                              : QPointF(40, 150);

    QPointF enemyFinal = b->battleEnemyItem->pos();

    // Start off-screen
    if (playerSprite)
        playerSprite->setX(playerFinal.x() - 500);

    b->battleEnemyItem->setX(enemyFinal.x() + 500);

    // Player slide
    QVariantAnimation *playerSlide = new QVariantAnimation();
    playerSlide->setDuration(750);
    playerSlide->setStartValue(playerFinal.x() - 500);
    playerSlide->setEndValue(playerFinal.x());
    playerSlide->setEasingCurve(QEasingCurve::OutBack);

    QObject::connect(playerSlide, &QVariantAnimation::valueChanged,
                     [=](const QVariant &v)
                     {
                         if (playerSprite)
                             playerSprite->setX(v.toReal());
                     });

    // Enemy slide
    QVariantAnimation *enemySlide = new QVariantAnimation();
    enemySlide->setDuration(750);
    enemySlide->setStartValue(b->battleEnemyItem->x());
    enemySlide->setEndValue(enemyFinal.x());
    enemySlide->setEasingCurve(QEasingCurve::OutBack);

    QObject::connect(enemySlide, &QVariantAnimation::valueChanged,
                     [=](const QVariant &v)
                     {
                         b->battleEnemyItem->setX(v.toReal());
                     });

    // Start both after a tiny delay (feels nicer)
    QTimer::singleShot(100, [=]()
                       {
                           playerSlide->start(QAbstractAnimation::DeleteWhenStopped);
                           enemySlide->start(QAbstractAnimation::DeleteWhenStopped);
                       });
}

// ================================================
// 3. SLIDE-IN COMMAND MENU
// ================================================
void Animations_BT::slideInCommandMenu(BattleSequence *b)
{
    if (!b || !b->commandBoxSprite) return;

    int px[4] = {25, 100, 25, 100};
    int py[4] = {18, 18, 42, 42};

    QVariantAnimation *anim = new QVariantAnimation();
    anim->setDuration(250);
    anim->setStartValue(300);
    anim->setEndValue(b->commandBoxSprite->pos().y());
    anim->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(anim, &QVariantAnimation::valueChanged,
                     [=](const QVariant &v)
                     {
                         int newY = v.toInt();
                         b->commandBoxSprite->setY(newY);

                         for (int i = 0; i < 4; ++i)
                         {
                             b->battleMenuOptions[i]->setPos(
                                 b->commandBoxSprite->pos().x() + px[i] * b->commandBoxSprite->scale(),
                                 newY + py[i] * b->commandBoxSprite->scale());
                         }

                         b->battleCursorSprite->setY(
                             b->battleMenuOptions[b->battleMenuIndex]->pos().y() - 5);
                     });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// ================================================
// 4. SLIDE-OUT COMMAND MENU
// ================================================
void Animations_BT::slideOutCommandMenu(BattleSequence *b, std::function<void()> done)
{
    if (!b || !b->commandBoxSprite) return;

    int px[4] = {25,100,25,100};
    int py[4] = {18,18,42,42};

    QVariantAnimation *anim = new QVariantAnimation();
    anim->setDuration(200);
    anim->setStartValue(b->commandBoxSprite->pos().y());
    anim->setEndValue(300);
    anim->setEasingCurve(QEasingCurve::InCubic);

    QObject::connect(anim, &QVariantAnimation::valueChanged,
                     [=](const QVariant &v)
                     {
                         int newY = v.toInt();
                         b->commandBoxSprite->setY(newY);

                         for (int i = 0; i < 4; ++i)
                         {
                             b->battleMenuOptions[i]->setPos(
                                 b->commandBoxSprite->pos().x() + px[i] * b->commandBoxSprite->scale(),
                                 newY + py[i] * b->commandBoxSprite->scale());
                         }

                         b->battleCursorSprite->setY(
                             b->battleMenuOptions[b->battleMenuIndex]->pos().y() - 5);
                     });

    if (done)
        QObject::connect(anim, &QVariantAnimation::finished, done);

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// ================================================
// 5. MENU SELECTION BOUNCE
// ================================================
void Animations_BT::animateMenuSelection(BattleSequence *b, int index)
{
    if (!b || !b->battleCursorSprite) return;

    int targetY = b->battleMenuOptions[index]->pos().y() - 5;

    QVariantAnimation *anim = new QVariantAnimation();
    anim->setDuration(80);
    anim->setStartValue(targetY);
    anim->setKeyValueAt(0.4, targetY - 3);
    anim->setEndValue(targetY);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(anim, &QVariantAnimation::valueChanged,
                     [=](const QVariant &v)
                     {
                         b->battleCursorSprite->setY(v.toInt());
                     });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

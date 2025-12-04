#include "battle/battle_animations.h"
#include "battle_sequence.h"
#include <QPainterPath>
#include <QVariantAnimation>
#include <QTimer>

BattleAnimations::BattleAnimations(QObject *parent)
    : QObject(parent)
{
}


// ================================================
// 1. BATTLE ZOOM REVEAL
// ================================================
void BattleAnimations::battleZoomReveal(BattleSequence *b)
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
// 2. ENTRANCE ANIMATION
// ================================================
void BattleAnimations::animateBattleEntrances(BattleSequence *b)
{
    if (!b || !b->battleEnemyItem) return;

    QGraphicsScene *scene = b->getScene();

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

    // Slide-ins
    QPointF playerFinal =
        b->battlePlayerPokemonItem ? b->battlePlayerPokemonItem->pos() :
            (b->battleTrainerItem ? b->battleTrainerItem->pos() : QPointF(40, 150));

    QPointF enemyFinal = b->battleEnemyItem->pos();

    // Move start positions
    if (b->battlePlayerPokemonItem)
        b->battlePlayerPokemonItem->setX(playerFinal.x() - 500);
    else if (b->battleTrainerItem)
        b->battleTrainerItem->setX(playerFinal.x() - 500);

    b->battleEnemyItem->setX(enemyFinal.x() + 500);

    // Player slide
    QVariantAnimation *playerSlide = new QVariantAnimation();
    playerSlide->setDuration(750);
    qreal playerStartX =
        b->battlePlayerPokemonItem ? b->battlePlayerPokemonItem->x() :
            (b->battleTrainerItem ? b->battleTrainerItem->x() : playerFinal.x() - 500);

    playerSlide->setStartValue(playerStartX);
    playerSlide->setEndValue(playerFinal.x());
    playerSlide->setEasingCurve(QEasingCurve::OutBack);

    QObject::connect(playerSlide, &QVariantAnimation::valueChanged,
                     [=](const QVariant &v)
                     {
                         if (b->battlePlayerPokemonItem)
                             b->battlePlayerPokemonItem->setX(v.toReal());
                         else if (b->battleTrainerItem)
                             b->battleTrainerItem->setX(v.toReal());
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

    QTimer::singleShot(100, [=]()
                       {
                           playerSlide->start(QAbstractAnimation::DeleteWhenStopped);
                           enemySlide->start(QAbstractAnimation::DeleteWhenStopped);
                       });
}

// ================================================
// 3. SLIDE-IN COMMAND MENU
// ================================================
void BattleAnimations::slideInCommandMenu(BattleSequence *b)
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
void BattleAnimations::slideOutCommandMenu(BattleSequence *b, std::function<void()> done)
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
void BattleAnimations::animateMenuSelection(BattleSequence *b, int index)
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


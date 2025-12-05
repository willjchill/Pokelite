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

    QVector<QPixmap> frames;
    for (int i = 1; i <= 5; ++i) {
        QString path = QString(":/assets/battle/battle_player%1.png").arg(i);
        QPixmap px(path);
        if (!px.isNull())
            frames.push_back(px);
    }

    if (frames.isEmpty()) {
        if (onFinished) onFinished();
        return;
    }

    QGraphicsPixmapItem *throwSprite = scene->addPixmap(frames[0]);
    throwSprite->setScale(b->battleTrainerItem->scale());
    throwSprite->setZValue(b->battleTrainerItem->zValue() + 1);
    throwSprite->setPos(b->battleTrainerItem->pos());
    b->battleTrainerItem->setVisible(false);

    QPixmap pokeballPx(":/assets/battle/pokeball.png");
    if (pokeballPx.isNull()) {
        pokeballPx = QPixmap(24, 24);
        pokeballPx.fill(Qt::transparent);
        QPainter painter(&pokeballPx);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 50, 50));
        painter.drawChord(1, 1, 22, 22, 0 * 16, 180 * 16);

        painter.setBrush(Qt::white);
        painter.drawChord(1, 1, 22, 22, 180 * 16, 180 * 16);

        painter.setPen(QPen(Qt::black, 2.5));
        painter.drawLine(1, 12, 23, 12);

        painter.setBrush(Qt::white);
        painter.drawEllipse(9, 9, 6, 6);

        painter.setPen(QPen(Qt::black, 1.8));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(1, 1, 22, 22);
    }

    QGraphicsPixmapItem *pokeball = scene->addPixmap(pokeballPx);
    pokeball->setScale(1.0);
    pokeball->setZValue(999);
    pokeball->setVisible(false);
    pokeball->setPos(throwSprite->x() + 60, throwSprite->y() + 30);

    int frameIndex = 0;
    QTimer *timer = new QTimer(b);
    timer->setInterval(100);

    QObject::connect(timer, &QTimer::timeout, b,
                     [=]() mutable
                     {
                         ++frameIndex;

                         if (frameIndex == 3) {
                             pokeball->setVisible(true);

                             QVariantAnimation *throwAnim = new QVariantAnimation(b);
                             throwAnim->setDuration(450);
                             throwAnim->setStartValue(0.0);
                             throwAnim->setEndValue(1.0);
                             throwAnim->setEasingCurve(QEasingCurve::InOutQuad);

                             qreal startX = pokeball->x();
                             qreal startY = pokeball->y();
                             qreal endX = b->battlePlayerPokemonItem->x() + 50;
                             qreal endY = b->battlePlayerPokemonItem->y() + 20;

                             QObject::connect(throwAnim, &QVariantAnimation::valueChanged,
                                              [=](const QVariant &v) {
                                                  qreal t = v.toReal();
                                                  qreal arc = std::sin(t * M_PI) * 35;
                                                  pokeball->setX(startX + (endX - startX) * t);
                                                  pokeball->setY(startY + (endY - startY) * t - arc);
                                                  pokeball->setRotation(t * 540);
                                              });

                             QObject::connect(throwAnim, &QVariantAnimation::finished,
                                              [=]() {
                                                  qreal ballCenterX = pokeball->x() + 12;
                                                  qreal ballCenterY = pokeball->y() + 12;

                                                  QGraphicsEllipseItem *openFlash = new QGraphicsEllipseItem(
                                                      ballCenterX - 40,
                                                      ballCenterY - 40,
                                                      80, 80
                                                      );
                                                  openFlash->setBrush(Qt::white);
                                                  openFlash->setPen(Qt::NoPen);
                                                  openFlash->setOpacity(0.0);
                                                  openFlash->setZValue(1001);
                                                  scene->addItem(openFlash);

                                                  QVariantAnimation *openAnim = new QVariantAnimation(b);
                                                  openAnim->setDuration(400);
                                                  openAnim->setStartValue(0.0);
                                                  openAnim->setKeyValueAt(0.15, 1.0);
                                                  openAnim->setKeyValueAt(0.5, 0.8);
                                                  openAnim->setEndValue(0.0);

                                                  QObject::connect(openAnim, &QVariantAnimation::valueChanged,
                                                                   [=](const QVariant &v) {
                                                                       qreal t = v.toReal();
                                                                       openFlash->setOpacity(t);
                                                                       qreal size = 80 + t * 40;
                                                                       openFlash->setRect(
                                                                           ballCenterX - size/2,
                                                                           ballCenterY - size/2,
                                                                           size, size
                                                                           );
                                                                   });

                                                  QObject::connect(openAnim, &QVariantAnimation::finished,
                                                                   [=]() {
                                                                       scene->removeItem(openFlash);
                                                                       delete openFlash;
                                                                   });

                                                  openAnim->start(QAbstractAnimation::DeleteWhenStopped);

                                                  for (int i = 0; i < 5; ++i) {
                                                      QGraphicsEllipseItem *particle = new QGraphicsEllipseItem(
                                                          ballCenterX - 3,
                                                          ballCenterY - 3,
                                                          6, 6
                                                          );
                                                      particle->setBrush(QColor(255, 255, 200));
                                                      particle->setPen(Qt::NoPen);
                                                      particle->setZValue(1000);
                                                      particle->setOpacity(1.0);
                                                      scene->addItem(particle);

                                                      qreal angle = (i * 72) * M_PI / 180.0;
                                                      qreal dist = 50;

                                                      QVariantAnimation *particleAnim = new QVariantAnimation(b);
                                                      particleAnim->setDuration(350);
                                                      particleAnim->setStartValue(0.0);
                                                      particleAnim->setEndValue(1.0);

                                                      QObject::connect(particleAnim, &QVariantAnimation::valueChanged,
                                                                       [=](const QVariant &v) {
                                                                           qreal t = v.toReal();
                                                                           particle->setPos(
                                                                               ballCenterX - 3 + std::cos(angle) * dist * t,
                                                                               ballCenterY - 3 + std::sin(angle) * dist * t
                                                                               );
                                                                           particle->setOpacity(1.0 - t);
                                                                       });

                                                      QObject::connect(particleAnim, &QVariantAnimation::finished,
                                                                       [=]() {
                                                                           scene->removeItem(particle);
                                                                           delete particle;
                                                                       });

                                                      QTimer::singleShot(i * 40, [=]() {
                                                          particleAnim->start(QAbstractAnimation::DeleteWhenStopped);
                                                      });
                                                  }

                                                  scene->removeItem(pokeball);
                                                  delete pokeball;
                                              });

                             throwAnim->start(QAbstractAnimation::DeleteWhenStopped);
                         }

                         if (frameIndex >= frames.size()) {
                             timer->stop();
                             scene->removeItem(throwSprite);
                             delete throwSprite;

                             if (onFinished) onFinished();
                             timer->deleteLater();
                             return;
                         }

                         throwSprite->setPixmap(frames[frameIndex]);
                     });

    timer->start();
}

void Animations_BT::animateAttackImpact(BattleSequence *b, bool isPlayerAttacking)
{
    if (!b || !b->getScene()) return;

    QGraphicsScene *scene = b->getScene();
    QGraphicsPixmapItem *attacker = isPlayerAttacking ? b->battlePlayerPokemonItem : b->battleEnemyItem;
    QGraphicsPixmapItem *target = isPlayerAttacking ? b->battleEnemyItem : b->battlePlayerPokemonItem;

    if (!attacker || !target) return;

    qreal originalX = attacker->x();
    qreal lungeDistance = isPlayerAttacking ? 50 : -50;

    QVariantAnimation *lunge = new QVariantAnimation();
    lunge->setDuration(100);
    lunge->setStartValue(originalX);
    lunge->setEndValue(originalX + lungeDistance);
    lunge->setEasingCurve(QEasingCurve::OutCubic);

    QObject::connect(lunge, &QVariantAnimation::valueChanged,
                     [=](const QVariant &v) {
                         attacker->setX(v.toReal());
                     });

    QObject::connect(lunge, &QVariantAnimation::finished,
                     [=]() {
                         qreal targetCenterX = target->x() + (target->boundingRect().width() * target->scale()) / 2;
                         qreal targetCenterY = target->y() + (target->boundingRect().height() * target->scale()) / 2;

                         QGraphicsEllipseItem *impactRing = new QGraphicsEllipseItem(
                             targetCenterX - 10,
                             targetCenterY - 10,
                             20, 20
                             );
                         impactRing->setBrush(Qt::NoBrush);
                         impactRing->setPen(QPen(QColor(255, 200, 0), 4));
                         impactRing->setZValue(999);
                         impactRing->setOpacity(1.0);
                         scene->addItem(impactRing);

                         QVariantAnimation *ringAnim = new QVariantAnimation();
                         ringAnim->setDuration(300);
                         ringAnim->setStartValue(0.0);
                         ringAnim->setEndValue(1.0);

                         QObject::connect(ringAnim, &QVariantAnimation::valueChanged,
                                          [=](const QVariant &v) {
                                              qreal t = v.toReal();
                                              qreal size = 20 + t * 80;
                                              impactRing->setRect(
                                                  targetCenterX - size/2,
                                                  targetCenterY - size/2,
                                                  size, size
                                                  );
                                              impactRing->setOpacity(1.0 - t);
                                          });

                         QObject::connect(ringAnim, &QVariantAnimation::finished,
                                          [=]() {
                                              scene->removeItem(impactRing);
                                              delete impactRing;
                                          });

                         ringAnim->start(QAbstractAnimation::DeleteWhenStopped);

                         for (int i = 0; i < 8; ++i) {
                             QGraphicsEllipseItem *spark = new QGraphicsEllipseItem(
                                 targetCenterX - 4,
                                 targetCenterY - 4,
                                 8, 8
                                 );
                             spark->setBrush(QColor(255, 150, 50));
                             spark->setPen(Qt::NoPen);
                             spark->setZValue(998);
                             spark->setOpacity(1.0);
                             scene->addItem(spark);

                             qreal angle = (i * 45) * M_PI / 180.0;
                             qreal distance = 40 + QRandomGenerator::global()->bounded(30);

                             QVariantAnimation *sparkAnim = new QVariantAnimation();
                             sparkAnim->setDuration(300);
                             sparkAnim->setStartValue(0.0);
                             sparkAnim->setEndValue(1.0);

                             QObject::connect(sparkAnim, &QVariantAnimation::valueChanged,
                                              [=](const QVariant &v) {
                                                  qreal t = v.toReal();
                                                  spark->setPos(
                                                      targetCenterX - 4 + std::cos(angle) * distance * t,
                                                      targetCenterY - 4 + std::sin(angle) * distance * t
                                                      );
                                                  spark->setOpacity(1.0 - t);
                                                  qreal scale = 1.0 + t * 0.5;
                                                  spark->setScale(scale);
                                              });

                             QObject::connect(sparkAnim, &QVariantAnimation::finished,
                                              [=]() {
                                                  scene->removeItem(spark);
                                                  delete spark;
                                              });

                             sparkAnim->start(QAbstractAnimation::DeleteWhenStopped);
                         }

                         QGraphicsEllipseItem *coreFlash = new QGraphicsEllipseItem(
                             targetCenterX - 30,
                             targetCenterY - 30,
                             60, 60
                             );
                         coreFlash->setBrush(Qt::white);
                         coreFlash->setPen(Qt::NoPen);
                         coreFlash->setZValue(1000);
                         coreFlash->setOpacity(0.0);
                         scene->addItem(coreFlash);

                         QVariantAnimation *flashAnim = new QVariantAnimation();
                         flashAnim->setDuration(200);
                         flashAnim->setStartValue(0.0);
                         flashAnim->setKeyValueAt(0.15, 1.0);
                         flashAnim->setKeyValueAt(0.6, 0.6);
                         flashAnim->setEndValue(0.0);

                         QObject::connect(flashAnim, &QVariantAnimation::valueChanged,
                                          [=](const QVariant &v) {
                                              coreFlash->setOpacity(v.toReal());
                                          });

                         QObject::connect(flashAnim, &QVariantAnimation::finished,
                                          [=]() {
                                              scene->removeItem(coreFlash);
                                              delete coreFlash;
                                          });

                         flashAnim->start(QAbstractAnimation::DeleteWhenStopped);

                         qreal targetOriginalX = target->x();
                         qreal targetOriginalY = target->y();
                         QVariantAnimation *shake = new QVariantAnimation();
                         shake->setDuration(250);
                         shake->setStartValue(0);
                         shake->setKeyValueAt(0.15, 12);
                         shake->setKeyValueAt(0.3, -12);
                         shake->setKeyValueAt(0.45, 8);
                         shake->setKeyValueAt(0.6, -8);
                         shake->setKeyValueAt(0.75, 4);
                         shake->setKeyValueAt(0.9, -4);
                         shake->setEndValue(0);

                         QObject::connect(shake, &QVariantAnimation::valueChanged,
                                          [=](const QVariant &v) {
                                              int offset = v.toInt();
                                              target->setX(targetOriginalX + offset);
                                              target->setY(targetOriginalY + (offset % 3 == 0 ? 3 : -3));
                                          });

                         QObject::connect(shake, &QVariantAnimation::finished,
                                          [=]() {
                                              target->setPos(targetOriginalX, targetOriginalY);
                                          });

                         shake->start(QAbstractAnimation::DeleteWhenStopped);

                         QVariantAnimation *returnAnim = new QVariantAnimation();
                         returnAnim->setDuration(100);
                         returnAnim->setStartValue(attacker->x());
                         returnAnim->setEndValue(originalX);
                         returnAnim->setEasingCurve(QEasingCurve::InCubic);

                         QObject::connect(returnAnim, &QVariantAnimation::valueChanged,
                                          [=](const QVariant &v) {
                                              attacker->setX(v.toReal());
                                          });

                         returnAnim->start(QAbstractAnimation::DeleteWhenStopped);
                     });

    lunge->start(QAbstractAnimation::DeleteWhenStopped);
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

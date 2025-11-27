// battle_animations.cpp
// ANIMATION FUNCTIONS ONLY

#include "mainwindow.h"
#include <QPainterPath>
#include <QVariantAnimation>
#include <QTimer>

void MainWindow::battleZoomReveal()
{
    QGraphicsPathItem *mask = new QGraphicsPathItem();
    mask->setBrush(Qt::black);
    mask->setPen(Qt::NoPen);
    mask->setZValue(9999);
    battleScene->addItem(mask);

    QPainterPath full;
    full.addRect(0,0,480,272);

    QVariantAnimation *anim=new QVariantAnimation(this);
    anim->setDuration(2000);
    anim->setStartValue(0);
    anim->setEndValue(1100);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(anim, &QVariantAnimation::valueChanged, this,
            [=](const QVariant &v){
                qreal r = v.toReal();
                QPainterPath hole;
                hole.addEllipse(240-r,136-r,2*r,2*r);

                QPainterPath reveal = full.subtracted(hole);
                mask->setPath(reveal);
            });

    connect(anim, &QVariantAnimation::finished, this,
            [=](){
                battleScene->removeItem(mask);
                delete mask;
            });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateBattleEntrances()
{
    if (!battleEnemyItem) return;

    QGraphicsPathItem *mask=new QGraphicsPathItem();
    mask->setBrush(Qt::black);
    mask->setPen(Qt::NoPen);
    mask->setZValue(9999);
    battleScene->addItem(mask);

    QPainterPath full;
    full.addRect(0,0,480,272);

    QVariantAnimation *circle=new QVariantAnimation(this);
    circle->setDuration(900);
    circle->setStartValue(0.0);
    circle->setEndValue(450.0);
    circle->setEasingCurve(QEasingCurve::OutCubic);

    connect(circle,&QVariantAnimation::valueChanged,this,
            [=](const QVariant &v){
                qreal r=v.toReal();
                QPainterPath hole;
                hole.addEllipse(240-r,136-r,2*r,2*r);
                QPainterPath maskPath = full.subtracted(hole);
                mask->setPath(maskPath);
            });

    connect(circle,&QVariantAnimation::finished,this,
            [=](){
                battleScene->removeItem(mask);
                delete mask;
            });

    circle->start(QAbstractAnimation::DeleteWhenStopped);

    QPointF playerFinal = battlePlayerPokemonItem ? battlePlayerPokemonItem->pos() : 
                          (battleTrainerItem ? battleTrainerItem->pos() : QPointF(40, 150));
    QPointF enemyFinal=battleEnemyItem->pos();

    if (battlePlayerPokemonItem) {
        battlePlayerPokemonItem->setX(playerFinal.x()-500);
    } else if (battleTrainerItem) {
        battleTrainerItem->setX(playerFinal.x()-500);
    }
    battleEnemyItem->setX(enemyFinal.x()+500);

    QVariantAnimation *playerSlide=new QVariantAnimation(this);
    playerSlide->setDuration(750);
    qreal playerStartX = battlePlayerPokemonItem ? battlePlayerPokemonItem->x() : 
                        (battleTrainerItem ? battleTrainerItem->x() : playerFinal.x() - 500);
    playerSlide->setStartValue(playerStartX);
    playerSlide->setEndValue(playerFinal.x());
    playerSlide->setEasingCurve(QEasingCurve::OutBack);

    connect(playerSlide,&QVariantAnimation::valueChanged,this,
            [=](const QVariant &v){
                if (battlePlayerPokemonItem) {
                    battlePlayerPokemonItem->setX(v.toReal());
                } else if (battleTrainerItem) {
                    battleTrainerItem->setX(v.toReal());
                }
            });

    QVariantAnimation *enemySlide=new QVariantAnimation(this);
    enemySlide->setDuration(750);
    enemySlide->setStartValue(battleEnemyItem->x());
    enemySlide->setEndValue(enemyFinal.x());
    enemySlide->setEasingCurve(QEasingCurve::OutBack);

    connect(enemySlide,&QVariantAnimation::valueChanged,this,
            [&](const QVariant &v){
                battleEnemyItem->setX(v.toReal());
            });

    QTimer::singleShot(100,this,[=](){
        playerSlide->start(QAbstractAnimation::DeleteWhenStopped);
        enemySlide->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void MainWindow::slideInCommandMenu()
{
    int px[4]={25,100,25,100};
    int py[4]={18,18,42,42};

    int targetY = commandBoxSprite->pos().y();

    QVariantAnimation *anim=new QVariantAnimation(this);
    anim->setDuration(250);
    anim->setStartValue(300);
    anim->setEndValue(targetY);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(anim,&QVariantAnimation::valueChanged,this,
            [=](const QVariant &val){
                int newY = val.toInt();
                commandBoxSprite->setY(newY);

                for(int i=0;i<4;i++){
                    battleMenuOptions[i]->setPos(
                        commandBoxSprite->pos().x()+px[i]*commandBoxSprite->scale(),
                        newY + py[i]*commandBoxSprite->scale()
                        );
                }

                battleCursorSprite->setY(
                    battleMenuOptions[battleMenuIndex]->pos().y()-5
                    );
            });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::slideOutCommandMenu(std::function<void()> onFinished)
{
    int px[4]={25,100,25,100};
    int py[4]={18,18,42,42};

    QVariantAnimation *anim=new QVariantAnimation(this);
    anim->setDuration(200);
    anim->setStartValue(commandBoxSprite->pos().y());
    anim->setEndValue(300);
    anim->setEasingCurve(QEasingCurve::InCubic);

    connect(anim, &QVariantAnimation::valueChanged, this,
            [=](const QVariant &val){
                int newY = val.toInt();
                commandBoxSprite->setY(newY);

                for(int i=0;i<4;i++){
                    battleMenuOptions[i]->setPos(
                        commandBoxSprite->pos().x()+px[i]*commandBoxSprite->scale(),
                        newY + py[i]*commandBoxSprite->scale()
                        );
                }

                battleCursorSprite->setY(
                    battleMenuOptions[battleMenuIndex]->pos().y()-5
                    );
            });

    if (onFinished)
        connect(anim,&QVariantAnimation::finished,this,onFinished);

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateMenuSelection(int index)
{
    if (!battleCursorSprite) return;

    int targetY = battleMenuOptions[index]->pos().y()-5;

    QVariantAnimation *anim=new QVariantAnimation(this);
    anim->setDuration(80);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    anim->setStartValue(targetY);
    anim->setKeyValueAt(0.4,targetY-3);
    anim->setEndValue(targetY);

    connect(anim,&QVariantAnimation::valueChanged,this,
            [=](const QVariant &v){
                battleCursorSprite->setY(v.toInt());
            });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateCommandSelection(int index)
{
    if (battleCursorSprite) {
        QVariantAnimation *bounce=new QVariantAnimation(this);
        bounce->setDuration(120);
        bounce->setStartValue(battleCursorSprite->y());
        bounce->setKeyValueAt(0.5,battleCursorSprite->y()-6);
        bounce->setEndValue(battleCursorSprite->y());
        bounce->setEasingCurve(QEasingCurve::OutCubic);

        connect(bounce,&QVariantAnimation::valueChanged,this,
                [&](const QVariant &v){
                    battleCursorSprite->setY(v.toInt());
                });

        bounce->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QGraphicsRectItem *flash=new QGraphicsRectItem();
    flash->setRect(battleMenuOptions[index]->pos().x()-5,
                   battleMenuOptions[index]->pos().y()-2,
                   60,20);
    flash->setBrush(Qt::white);
    flash->setPen(Qt::NoPen);
    flash->setOpacity(0.0);
    flash->setZValue(10);
    battleScene->addItem(flash);

    QVariantAnimation *flashAnim=new QVariantAnimation(this);
    flashAnim->setDuration(180);
    flashAnim->setStartValue(0.0);
    flashAnim->setKeyValueAt(0.3,1.0);
    flashAnim->setEndValue(0.0);

    connect(flashAnim,&QVariantAnimation::valueChanged,this,
            [flash](const QVariant &v){
                flash->setOpacity(v.toReal());
            });

    connect(flashAnim,&QVariantAnimation::finished,this,
            [flash](){
                flash->scene()->removeItem(flash);
                delete flash;
            });

    flashAnim->start(QAbstractAnimation::DeleteWhenStopped);

    QVariantAnimation *slide=new QVariantAnimation(this);
    slide->setDuration(260);
    slide->setStartValue(commandBoxSprite->y());
    slide->setEndValue(commandBoxSprite->y()+160);
    slide->setEasingCurve(QEasingCurve::InQuad);

    connect(slide,&QVariantAnimation::valueChanged,this,
            [&](const QVariant &v){
                qreal newY=v.toReal();
                commandBoxSprite->setY(newY);

                for(int i=0;i<battleMenuOptions.size();i++){
                    battleMenuOptions[i]->setY(
                        newY + (i<2?18:42) * commandBoxSprite->scale()
                        );
                }

                battleCursorSprite->setY(
                    battleMenuOptions[battleMenuIndex]->y() - 5
                    );
            });

    slide->start(QAbstractAnimation::DeleteWhenStopped);

    QTimer::singleShot(260,this,[=](){
        QString commands[4]={
            "You chose FIGHT!",
            "You opened your BAG...",
            "You check your POK\u00e9MON...",
            "You attempt to RUN..."
        };

        fullBattleText = commands[index];
        battleTextItem->setPlainText("");
        battleTextIndex = 0;
        battleTextTimer.start(22);
    });
}

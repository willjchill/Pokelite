#ifndef ANIMATIONS_BT_H
#define ANIMATIONS_BT_H

#include <QObject>
#include <QVariantAnimation>
#include <QTimer>
#include <functional>

class BattleSequence; // forward declare

class Animations_BT : public QObject
{
    Q_OBJECT
public:
    explicit Animations_BT(QObject* parent = nullptr);

    // Every function receives BattleSequence* and uses ITS variables
    void animateTrainerThrow(BattleSequence* b, std::function<void()> onFinished);
    void battleZoomReveal(BattleSequence *b);
    void animateBattleEntrances(BattleSequence *b);
    void slideInCommandMenu(BattleSequence *b);
    void slideOutCommandMenu(BattleSequence *b, std::function<void()> finished = nullptr);
    void animateMenuSelection(BattleSequence *b, int index);
    void playTrainerThrowFrames(BattleSequence* b, std::function<void()> onFinished);

};

#endif // ANIMATIONS_BT_H

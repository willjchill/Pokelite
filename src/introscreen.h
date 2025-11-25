#ifndef INTROSCREEN_H
#define INTROSCREEN_H

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QPixmap>
#include <QVector>

class IntroScreen : public QWidget
{
    Q_OBJECT

public:
    explicit IntroScreen(QWidget *parent = nullptr);
    ~IntroScreen();

signals:
    void introFinished();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void advanceFrame();

private:
    QLabel *imageLabel;
    QTimer frameTimer;

    enum Phase {
        PHASE_COPYRIGHT,
        PHASE_GRASS,
        PHASE_TITLE
    };

    Phase phase;

    QVector<QPixmap> copyrightFrames;
    QVector<QPixmap> grassFrames;
    QVector<QPixmap> titleFrames;

    int frameIndex;
    int phaseTicks;

    void switchPhase(Phase newPhase);
    void loadFrames();
};

#endif // INTROSCREEN_H

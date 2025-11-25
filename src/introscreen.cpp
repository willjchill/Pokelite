#include "introscreen.h"
#include <QKeyEvent>
#include <QShowEvent>
#include <QApplication>
#include <QDebug>

IntroScreen::IntroScreen(QWidget *parent)
    : QWidget(parent),
    frameIndex(0),
    phaseTicks(0),
    gamepadThread(nullptr)
{
    setFixedSize(480, 272);
    
    // Set focus policy to ensure keyboard input works
    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    imageLabel = new QLabel(this);
    imageLabel->setFixedSize(480, 272);
    imageLabel->setScaledContents(true);

    loadFrames();
    switchPhase(PHASE_COPYRIGHT);

    connect(&frameTimer, &QTimer::timeout, this, &IntroScreen::advanceFrame);

    frameTimer.start(33);    // ~30 FPS

    // Initialize gamepad thread
    gamepadThread = new GamepadThread("/dev/input/event1", this);
    connect(gamepadThread, &GamepadThread::inputReceived, this, &IntroScreen::handleGamepadInput);
    gamepadThread->start();
}

IntroScreen::~IntroScreen()
{
    if (gamepadThread) {
        gamepadThread->stop();
        delete gamepadThread;
    }
}

void IntroScreen::loadFrames()
{
    // COPYRIGHT
    copyrightFrames.push_back(QPixmap(":/assets/intro/copyright/copyright_01.png"));

    // GRASS FRAMES
    grassFrames.push_back(QPixmap(":/assets/intro/grass/grass_01.png"));
    grassFrames.push_back(QPixmap(":/assets/intro/grass/grass_02.png"));
    grassFrames.push_back(QPixmap(":/assets/intro/grass/grass_03.png"));
    grassFrames.push_back(QPixmap(":/assets/intro/grass/grass_04.png"));

    // TITLE FRAMES
    titleFrames.push_back(QPixmap(":/assets/intro/title/title_01.png"));
    titleFrames.push_back(QPixmap(":/assets/intro/title/title_02.png"));
    titleFrames.push_back(QPixmap(":/assets/intro/title/title_03.png"));
    titleFrames.push_back(QPixmap(":/assets/intro/title/title_04.png"));
}

void IntroScreen::switchPhase(Phase newPhase)
{
    phase = newPhase;
    frameIndex = 0;
    phaseTicks = 0;

    switch (phase) {
    case PHASE_COPYRIGHT:
        imageLabel->setPixmap(copyrightFrames[0]);
        break;

    case PHASE_GRASS:
        imageLabel->setPixmap(grassFrames[0]);
        break;

    case PHASE_TITLE:
        imageLabel->setPixmap(titleFrames[0]);
        break;
    }
}

void IntroScreen::advanceFrame()
{
    switch (phase) {

    case PHASE_COPYRIGHT:
        // stay for ~2 seconds
        if (++phaseTicks >= 60) {
            switchPhase(PHASE_GRASS);
        }
        break;

    case PHASE_GRASS:
        // Slower: change frame every 6 ticks
        if (++phaseTicks >= 6) {
            frameIndex++;
            phaseTicks = 0;
        }

        if (frameIndex >= grassFrames.size()) {
            switchPhase(PHASE_TITLE);
            return;
        }

        imageLabel->setPixmap(grassFrames[frameIndex]);
        break;

    case PHASE_TITLE:
        // Slower: change every 6 ticks
        if (frameIndex < titleFrames.size() - 1) {
            if (++phaseTicks >= 6) {
                frameIndex++;
                phaseTicks = 0;
            }
        } else {
            // stay on last frame
        }

        imageLabel->setPixmap(titleFrames[frameIndex]);
        break;
    }
}

void IntroScreen::keyPressEvent(QKeyEvent *event)
{
    if (phase == PHASE_TITLE &&
        frameIndex == titleFrames.size() - 1 &&
        (event->key() == Qt::Key_Return ||
         event->key() == Qt::Key_Enter ||
         event->key() == Qt::Key_Space)) {

        emit introFinished();
        return;
    }

    QWidget::keyPressEvent(event);
}

void IntroScreen::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Ensure focus when window is shown
    setFocus();
}

void IntroScreen::handleGamepadInput(int type, int code, int value)
{
    // Only handle button presses for intro screen
    if (type == 1 && value == 1) { // EV_KEY, pressed
        if (code == 304) { // A button
            simulateKeyPress(Qt::Key_Return);
        } else if (code == 315) { // Start button
            simulateKeyPress(Qt::Key_Return);
        }
    }
}

void IntroScreen::simulateKeyPress(Qt::Key key)
{
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier);
    QApplication::postEvent(this, event);
}

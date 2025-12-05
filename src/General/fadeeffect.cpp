#include "fadeeffect.h"
#include <QPainter>

FadeEffect::FadeEffect(QWidget *parent)
    : QWidget(parent),
    opacity(0.0f),
    fadeStep(0.0f),
    isFadingOut(false)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setStyleSheet("background: transparent;");
    hide();

    connect(&fadeTimer, &QTimer::timeout, this, &FadeEffect::updateOpacity);
}

void FadeEffect::fadeOut(int duration)
{
    isFadingOut = true;
    opacity = 0.0f;
    fadeStep = 1.0f / (duration / 16.0f); // 16ms per frame

    setGeometry(parentWidget()->rect());
    show();
    raise();

    fadeTimer.start(16);
}

void FadeEffect::fadeIn(int duration)
{
    isFadingOut = false;
    opacity = 1.0f;
    fadeStep = 1.0f / (duration / 16.0f);

    setGeometry(parentWidget()->rect());
    show();
    raise();

    fadeTimer.start(16);
}

void FadeEffect::updateOpacity()
{
    if (isFadingOut) {
        opacity += fadeStep;
        if (opacity >= 1.0f) {
            opacity = 1.0f;
            fadeTimer.stop();
            emit fadeOutFinished();
        }
    } else {
        opacity -= fadeStep;
        if (opacity <= 0.0f) {
            opacity = 0.0f;
            fadeTimer.stop();
            hide();
            emit fadeInFinished();
        }
    }

    update();
}

void FadeEffect::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, static_cast<int>(opacity * 255)));
}

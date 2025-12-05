#ifndef FADEEFFECT_H
#define FADEEFFECT_H

#include <QWidget>
#include <QTimer>

class FadeEffect : public QWidget
{
    Q_OBJECT

public:
    explicit FadeEffect(QWidget *parent = nullptr);

    void fadeOut(int duration = 300);
    void fadeIn(int duration = 300);

signals:
    void fadeOutFinished();
    void fadeInFinished();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateOpacity();

private:
    QTimer fadeTimer;
    float opacity;
    float fadeStep;
    bool isFadingOut;
};

#endif

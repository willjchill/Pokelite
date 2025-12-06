#ifndef LORESCREEN_H
#define LORESCREEN_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QStringList>
#include <QTimer>
#include "../General/gamepad.h"

class LoreScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LoreScreen(QWidget *parent = nullptr);
    ~LoreScreen();

signals:
    void loreFinished();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void typeNextCharacter();
    void updatePromptGlow();
    void handleGamepadInput(int type, int code, int value);

private:
    void startTyping(const QString &text);
    void simulateKeyPress(Qt::Key key);

    QLabel *loreTextLabel;
    QLabel *promptLabel;
    QLabel *bgLabel;
    QStringList loreParts;
    int currentPart;

    QTimer typeTimer;
    QTimer glowTimer;
    QString currentFullText;
    QString currentDisplayText;
    int typeIndex;
    bool isTyping;
    float glowIntensity;
    bool glowIncreasing;
    
    // Gamepad support
    Gamepad *gamepadThread;
};

#endif

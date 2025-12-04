#ifndef LORESCREEN_H
#define LORESCREEN_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QStringList>
#include <QTimer>

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

private:
    void startTyping(const QString &text);

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
};

#endif

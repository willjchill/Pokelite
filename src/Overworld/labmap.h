#ifndef LABMAP_H
#define LABMAP_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QStringList>
#include <QKeyEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QSet>
#include "Player_OW.h"

class LabMap : public QWidget
{
    Q_OBJECT

public:
    explicit LabMap(QWidget *parent = nullptr);
    ~LabMap();

signals:
    void exitToOverworld();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void typeNextCharacter();
    void updatePromptGlow();
    void gameLoop();

private:
    void startDialogue();
    void startTyping(const QString &text);
    void handleMovement();
    void checkExitTransition();

    QGraphicsView *view;
    QGraphicsScene *scene;
    Player_OW *player;

    QLabel *nameBoxLabel;
    QLabel *nameLabel;
    QLabel *textBoxLabel;
    QLabel *dialogueLabel;
    QLabel *promptLabel;

    QStringList dialogueParts;
    int currentPart;

    QTimer typeTimer;
    QTimer glowTimer;
    QTimer gameTimer;

    QString currentFullText;
    QString currentDisplayText;
    int typeIndex;
    bool isTyping;
    float glowIntensity;
    bool glowIncreasing;

    bool dialogueActive;
    bool dialogueFinished;

    QSet<int> keysPressed;
    float speed;
};

#endif

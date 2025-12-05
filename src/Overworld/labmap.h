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
#include <QImage>
#include <QVector>
#include "Player_OW.h"
#include <cmath>

class LabMap : public QWidget
{
    Q_OBJECT

public:
    explicit LabMap(QWidget *parent = nullptr);
    ~LabMap();
    void setPlayerSpawnPosition(const QPointF &pos);

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
    void checkNPCInteraction();
    bool isSolidPixel(int x, int y) const;
    bool isNPCPixel(int x, int y) const;
    void showStarterSelection();
    void updateStarterDisplay();
    void selectStarter();
    float shimmerPhase;
    void updateSelectionShimmer();

    QGraphicsView *view;
    QGraphicsScene *scene;
    Player_OW *player;

    QLabel *nameBoxLabel;
    QLabel *nameLabel;
    QLabel *textBoxLabel;
    QLabel *dialogueLabel;
    QLabel *promptLabel;

    QVector<QLabel*> starterBoxes;
    QVector<QLabel*> starterNames;
    QVector<QLabel*> starterTypes;
    QVector<QLabel*> starterSprites;
    QLabel *starterPromptLabel;

    QStringList dialogueParts;
    QStringList npcDialogueParts;
    int currentPart;

    QTimer typeTimer;
    QTimer glowTimer;
    QTimer gameTimer;
    QTimer selectionShimmerTimer;

    QString currentFullText;
    QString currentDisplayText;
    int typeIndex;
    bool isTyping;
    float glowIntensity;
    bool glowIncreasing;

    bool dialogueActive;
    bool dialogueFinished;
    bool isInitialDialogue;
    bool hasShownInitialDialogue;
    bool isSelectingStarter;
    int selectedStarterIndex;
    QString chosenStarter;

    QSet<int> keysPressed;
    float speed;

    QImage collisionMask;
    int bgXOffset;
    int bgYOffset;

    struct StarterPokemon {
        QString name;
        QString type;
        QString spritePath;
    };
    QVector<StarterPokemon> starters;
};

#endif

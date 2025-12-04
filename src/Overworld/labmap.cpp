#include "labmap.h"
#include "Player_OW.h"
#include <QKeyEvent>
#include <QShowEvent>
#include <QFont>

LabMap::LabMap(QWidget *parent)
    : QWidget(parent),
    currentPart(0),
    typeIndex(0),
    isTyping(false),
    glowIntensity(0.0f),
    glowIncreasing(true),
    dialogueActive(false),
    dialogueFinished(false),
    speed(3.0f)
{
    setFixedSize(480, 272);
    setFocusPolicy(Qt::StrongFocus);

    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 480, 272);

    view = new QGraphicsView(scene, this);
    view->setGeometry(0, 0, 480, 272);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setFrameShape(QFrame::NoFrame);

    QPixmap bgPixmap = QPixmap(":/assets/intro/title/labBG.png");
    QPixmap scaledBg = bgPixmap.scaled(480, 272, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    int xOffset = (480 - scaledBg.width()) / 2;
    int yOffset = (272 - scaledBg.height()) / 2;

    QGraphicsPixmapItem *bg = new QGraphicsPixmapItem(scaledBg);
    bg->setPos(xOffset, yOffset);
    scene->addItem(bg);

    player = new Player_OW();
    player->setScale(1.5);
    player->setDirection("back");
    player->setPosition(QPointF(240, 200));
    scene->addItem(player);

    nameBoxLabel = new QLabel(this);
    nameBoxLabel->setGeometry(30, 155, 130, 35);
    nameBoxLabel->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                                "stop:0 #FF8C00, stop:1 #FFA500); "
                                "border: 3px solid #FFF; "
                                "border-radius: 8px;");
    nameBoxLabel->hide();
    nameBoxLabel->raise();

    nameLabel = new QLabel(this);
    nameLabel->setGeometry(35, 160, 120, 25);
    nameLabel->setAlignment(Qt::AlignCenter);
    QFont nameFont;
    nameFont.setFamily("Courier");
    nameFont.setPointSize(11);
    nameFont.setStyleHint(QFont::TypeWriter);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);
    nameLabel->setStyleSheet("color: white; background: transparent;");
    nameLabel->setText("Prof. Oak");
    nameLabel->hide();
    nameLabel->raise();

    textBoxLabel = new QLabel(this);
    textBoxLabel->setGeometry(20, 190, 440, 70);
    textBoxLabel->setStyleSheet("background-color: rgba(255, 255, 255, 220); "
                                "border: 3px solid #333; "
                                "border-radius: 10px;");
    textBoxLabel->hide();
    textBoxLabel->raise();

    dialogueLabel = new QLabel(this);
    dialogueLabel->setGeometry(35, 200, 380, 50);
    dialogueLabel->setWordWrap(true);
    dialogueLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QFont font;
    font.setFamily("Courier");
    font.setPointSize(9);
    font.setStyleHint(QFont::TypeWriter);
    font.setBold(true);
    dialogueLabel->setFont(font);
    dialogueLabel->setStyleSheet("color: black; background: transparent;");
    dialogueLabel->hide();
    dialogueLabel->raise();

    promptLabel = new QLabel(this);
    promptLabel->setGeometry(380, 245, 70, 20);
    promptLabel->setAlignment(Qt::AlignRight);
    QFont promptFont;
    promptFont.setFamily("Courier");
    promptFont.setPointSize(8);
    promptFont.setStyleHint(QFont::TypeWriter);
    promptFont.setBold(true);
    promptLabel->setFont(promptFont);
    promptLabel->setText("▶");
    promptLabel->setStyleSheet("background: transparent;");
    promptLabel->hide();
    promptLabel->raise();

    dialogueParts.append("Hello there! Welcome to my laboratory!");

    dialogueParts.append("I've been studying Pokemon for over 30 years. "
                         "These magnificent creatures live alongside us in harmony.");

    dialogueParts.append("Some people raise Pokemon as pets, others use them "
                         "for battling. What will your story be?");

    dialogueParts.append("Your journey to become Champion won't be easy. "
                         "You'll need to collect 8 Gym Badges from the toughest trainers "
                         "in Bostonia!");

    dialogueParts.append("But first, you'll need a Pokemon of your own! "
                         "I'll give you one once you're ready to head out.");

    dialogueParts.append("The door is right there. When you're ready, "
                         "step outside and your adventure will begin!");

    connect(&typeTimer, &QTimer::timeout, this, &LabMap::typeNextCharacter);
    connect(&glowTimer, &QTimer::timeout, this, &LabMap::updatePromptGlow);
    connect(&gameTimer, &QTimer::timeout, this, &LabMap::gameLoop);

    gameTimer.start(16);
}

LabMap::~LabMap()
{
}

void LabMap::startDialogue()
{
    dialogueActive = true;
    currentPart = 0;
    nameBoxLabel->show();
    nameLabel->show();
    textBoxLabel->show();
    dialogueLabel->show();
    startTyping(dialogueParts[0]);
}

void LabMap::startTyping(const QString &text)
{
    currentFullText = text;
    currentDisplayText = "";
    typeIndex = 0;
    isTyping = true;
    promptLabel->hide();
    glowTimer.stop();
    dialogueLabel->setText("");
    typeTimer.start(30);
}

void LabMap::typeNextCharacter()
{
    if (typeIndex < currentFullText.length()) {
        currentDisplayText += currentFullText[typeIndex];
        dialogueLabel->setText(currentDisplayText);
        typeIndex++;
    } else {
        typeTimer.stop();
        isTyping = false;
        promptLabel->show();
        glowIntensity = 0.0f;
        glowIncreasing = true;
        glowTimer.start(30);
    }
}

void LabMap::updatePromptGlow()
{
    if (glowIncreasing) {
        glowIntensity += 0.05f;
        if (glowIntensity >= 1.0f) {
            glowIntensity = 1.0f;
            glowIncreasing = false;
        }
    } else {
        glowIntensity -= 0.05f;
        if (glowIntensity <= 0.3f) {
            glowIntensity = 0.3f;
            glowIncreasing = true;
        }
    }

    int intensity = 50 + (int)(155 * glowIntensity);

    QString style = QString("color: rgb(%1, %2, %3); background: transparent;")
                        .arg(intensity).arg(intensity).arg(intensity);
    promptLabel->setStyleSheet(style);
}

void LabMap::handleMovement()
{
    if (dialogueActive && !dialogueFinished) return;
    if (keysPressed.isEmpty()) {
        player->stopAnimation();
        return;
    }

    float dx = 0, dy = 0;
    QString direction;

    if (keysPressed.contains(Qt::Key_W) || keysPressed.contains(Qt::Key_Up)) {
        dy = -1;
        direction = "back";
    }
    if (keysPressed.contains(Qt::Key_S) || keysPressed.contains(Qt::Key_Down)) {
        dy = 1;
        direction = "front";
    }
    if (keysPressed.contains(Qt::Key_A) || keysPressed.contains(Qt::Key_Left)) {
        dx = -1;
        direction = "left";
    }
    if (keysPressed.contains(Qt::Key_D) || keysPressed.contains(Qt::Key_Right)) {
        dx = 1;
        direction = "right";
    }

    if (dx != 0 || dy != 0) {
        player->setDirection(direction);
        player->startAnimation();

        QPointF oldPos = player->getPosition();
        QPointF newPos = oldPos + QPointF(dx * speed, dy * speed);

        if (newPos.x() >= 0 && newPos.x() <= 480 - player->boundingRect().width() * player->scale() &&
            newPos.y() >= 0 && newPos.y() <= 272 - player->boundingRect().height() * player->scale()) {
            player->setPosition(newPos);
            checkExitTransition();
        }
    }
}

void LabMap::checkExitTransition()
{
    QPointF pos = player->getPosition();

    if (pos.y() >= 220 && pos.x() >= 200 && pos.x() <= 280) {
        gameTimer.stop();
        player->stopAnimation();
        emit exitToOverworld();
    }
}

void LabMap::gameLoop()
{
    handleMovement();
}

void LabMap::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    if (dialogueActive && !dialogueFinished) {
        if (event->key() == Qt::Key_Return ||
            event->key() == Qt::Key_Enter ||
            event->key() == Qt::Key_Space) {

            if (isTyping) {
                typeTimer.stop();
                dialogueLabel->setText(currentFullText);
                isTyping = false;
                promptLabel->show();
                glowIntensity = 0.0f;
                glowIncreasing = true;
                glowTimer.start(30);
                return;
            }

            currentPart++;

            if (currentPart < dialogueParts.size()) {
                startTyping(dialogueParts[currentPart]);
            } else {
                glowTimer.stop();
                dialogueActive = false;
                dialogueFinished = true;
                nameBoxLabel->hide();
                nameLabel->hide();
                textBoxLabel->hide();
                dialogueLabel->hide();
                promptLabel->hide();
            }
            return;
        }
    } else {
        switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_Up:
        case Qt::Key_S:
        case Qt::Key_Down:
        case Qt::Key_A:
        case Qt::Key_Left:
        case Qt::Key_D:
        case Qt::Key_Right:
            keysPressed.insert(event->key());
            break;
        }
    }

    QWidget::keyPressEvent(event);
}

void LabMap::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    keysPressed.remove(event->key());

    if (keysPressed.isEmpty()) {
        player->stopAnimation();
    }

    QWidget::keyReleaseEvent(event);
}

void LabMap::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    setFocus();
    startDialogue();
}

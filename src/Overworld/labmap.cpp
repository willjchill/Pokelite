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
    isInitialDialogue(true),
    isSelectingStarter(false),
    selectedStarterIndex(0),
    speed(1.5f),
    bgXOffset(0),
    bgYOffset(0),
    hasShownInitialDialogue(false),
    shimmerPhase(0.0f)
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

    bgXOffset = (480 - scaledBg.width()) / 2;
    bgYOffset = (272 - scaledBg.height()) / 2;

    QGraphicsPixmapItem *bg = new QGraphicsPixmapItem(scaledBg);
    bg->setPos(bgXOffset, bgYOffset);
    scene->addItem(bg);

    collisionMask = QImage(":/assets/intro/title/labCollision.png");
    if (!collisionMask.isNull()) {
        collisionMask = collisionMask.scaled(scaledBg.width(), scaledBg.height(),
                                             Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    player = new Player_OW();
    player->setScale(1.5);
    player->setDirection("back");
    player->setPosition(QPointF(220, 150));
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

    for (int i = 0; i < 3; i++) {
        QLabel *box = new QLabel(this);
        box->setGeometry(50 + i * 130, 60, 110, 140);
        box->setStyleSheet("background-color: rgba(255, 255, 255, 230); "
                           "border: 3px solid #999; "
                           "border-radius: 10px;");
        box->hide();
        box->raise();
        starterBoxes.append(box);

        QLabel *sprite = new QLabel(this);
        sprite->setGeometry(60 + i * 130, 70, 90, 90);
        sprite->setAlignment(Qt::AlignCenter);
        sprite->setScaledContents(false);
        sprite->setStyleSheet("background: transparent;");
        sprite->hide();
        sprite->raise();
        starterSprites.append(sprite);

        QLabel *name = new QLabel(this);
        name->setGeometry(50 + i * 130, 165, 110, 20);
        name->setAlignment(Qt::AlignCenter);
        QFont nameFont;
        nameFont.setFamily("Courier");
        nameFont.setPointSize(10);
        nameFont.setStyleHint(QFont::TypeWriter);
        nameFont.setBold(true);
        name->setFont(nameFont);
        name->setStyleSheet("color: #2C3E50; background: transparent;");
        name->hide();
        name->raise();
        starterNames.append(name);

        QLabel *type = new QLabel(this);
        type->setGeometry(60 + i * 130, 185, 90, 20);
        type->setAlignment(Qt::AlignCenter);
        QFont typeFont;
        typeFont.setFamily("Courier");
        typeFont.setPointSize(8);
        typeFont.setStyleHint(QFont::TypeWriter);
        typeFont.setBold(true);
        type->setFont(typeFont);
        type->setStyleSheet("color: white; border-radius: 5px; padding: 2px;");
        type->hide();
        type->raise();
        starterTypes.append(type);
    }

    starterPromptLabel = new QLabel(this);
    starterPromptLabel->setGeometry(90, 210, 300, 30);
    starterPromptLabel->setAlignment(Qt::AlignCenter);
    QFont starterPromptFont;
    starterPromptFont.setFamily("Courier");
    starterPromptFont.setPointSize(10);
    starterPromptFont.setStyleHint(QFont::TypeWriter);
    starterPromptFont.setBold(true);
    starterPromptLabel->setFont(starterPromptFont);
    starterPromptLabel->setText("◀ A/D to select | ENTER to confirm ▶");
    starterPromptLabel->setStyleSheet("color: white; "
                                      "background-color: rgba(0, 0, 0, 200); "
                                      "border: 3px solid white; "
                                      "border-radius: 8px; "
                                      "padding: 5px;");
    starterPromptLabel->hide();
    starterPromptLabel->raise();

    starters.append({"Bulbasaur", "GRASS", ":/Battle/assets/pokemon_sprites/001_bulbasaur/front.png"});
    starters.append({"Charmander", "FIRE", ":/Battle/assets/pokemon_sprites/004_charmander/front.png"});
    starters.append({"Squirtle", "WATER", ":/Battle/assets/pokemon_sprites/007_squirtle/front.png"});

    dialogueParts.append("There you are! I'm Professor Oak! You must be the new trainer everyone's been talking about!");

    dialogueParts.append("Welcome to the wonderful world of Pokemon! I've dedicated my entire life to studying these amazing creatures.");

    dialogueParts.append("Pokemon are incredible beings that live alongside humans. Some become our best friends, others help us in battle, and all of them make our world beautiful!");

    dialogueParts.append("Your dream is to become the Pokemon Champion of Bostonia, right? Well, it won't be easy! You'll need to defeat 8 Gym Leaders and prove your strength!");

    dialogueParts.append("But every great journey starts with a single step... and a partner Pokemon! It's time for you to choose your very first Pokemon!");

    npcDialogueParts.append("Oh hey, you're back! How's your journey going?");

    npcDialogueParts.append("Remember, type advantages are crucial in battle! "
                            "Fire beats Grass, Water beats Fire, and Grass beats Water.");

    npcDialogueParts.append("Don't forget to heal your Pokemon at Pokemon Centers! "
                            "They're completely free and can be found in every town.");

    npcDialogueParts.append("If you're ever stuck, try talking to people in towns. "
                            "They often have useful tips and information!");

    npcDialogueParts.append("Good luck out there, young trainer! "
                            "I believe you'll become a great Champion someday!");

    connect(&typeTimer, &QTimer::timeout, this, &LabMap::typeNextCharacter);
    connect(&glowTimer, &QTimer::timeout, this, &LabMap::updatePromptGlow);
    connect(&gameTimer, &QTimer::timeout, this, &LabMap::gameLoop);
    connect(&selectionShimmerTimer, &QTimer::timeout, this, &LabMap::updateSelectionShimmer);

    gameTimer.start(16);
}
LabMap::~LabMap()
{
}

bool LabMap::isSolidPixel(int x, int y) const
{
    if (collisionMask.isNull()) return false;

    int maskX = x - bgXOffset;
    int maskY = y - bgYOffset;

    if (maskX < 0 || maskX >= collisionMask.width() ||
        maskY < 0 || maskY >= collisionMask.height()) {
        return true;
    }

    QRgb pixel = collisionMask.pixel(maskX, maskY);
    return qRed(pixel) == 0 && qGreen(pixel) == 0 && qBlue(pixel) == 0;
}

bool LabMap::isNPCPixel(int x, int y) const
{
    if (collisionMask.isNull()) return false;

    int maskX = x - bgXOffset;
    int maskY = y - bgYOffset;

    if (maskX < 0 || maskX >= collisionMask.width() ||
        maskY < 0 || maskY >= collisionMask.height()) {
        return false;
    }

    QRgb pixel = collisionMask.pixel(maskX, maskY);
    return qRed(pixel) == 255 && qGreen(pixel) == 255 && qBlue(pixel) == 255;
}

void LabMap::showStarterSelection()
{
    isSelectingStarter = true;
    selectedStarterIndex = 0;

    nameBoxLabel->hide();
    nameLabel->hide();
    textBoxLabel->hide();
    dialogueLabel->hide();
    promptLabel->hide();

    for (int i = 0; i < 3; i++) {
        starterBoxes[i]->show();
        starterSprites[i]->show();
        starterNames[i]->show();
        starterTypes[i]->show();

        starterNames[i]->setText(starters[i].name);

        QPixmap sprite(starters[i].spritePath);
        if (!sprite.isNull()) {
            QPixmap scaledSprite = sprite.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            starterSprites[i]->setPixmap(scaledSprite);
        }

        if (starters[i].type == "GRASS") {
            starterTypes[i]->setText("GRASS");
            starterTypes[i]->setStyleSheet("color: white; background: #78C850; "
                                           "border-radius: 5px; padding: 2px;");
        } else if (starters[i].type == "FIRE") {
            starterTypes[i]->setText("FIRE");
            starterTypes[i]->setStyleSheet("color: white; background: #F08030; "
                                           "border-radius: 5px; padding: 2px;");
        } else if (starters[i].type == "WATER") {
            starterTypes[i]->setText("WATER");
            starterTypes[i]->setStyleSheet("color: white; background: #6890F0; "
                                           "border-radius: 5px; padding: 2px;");
        }
    }

    starterPromptLabel->show();
    shimmerPhase = 0.0f;
    selectionShimmerTimer.start(50);  // Start shimmer animation
    updateStarterDisplay();
}


void LabMap::updateStarterDisplay()
{
    for (int i = 0; i < 3; i++) {
        if (i == selectedStarterIndex) {
            // Calculate shimmer intensity (oscillates between 0.7 and 1.0)
            float shimmer = 0.85f + 0.15f * sin(shimmerPhase);

            // Create gradient gold colors
            int r1 = static_cast<int>(255 * shimmer);
            int g1 = static_cast<int>(215 * shimmer);
            int b1 = static_cast<int>(0 * shimmer);

            int r2 = static_cast<int>(218 * shimmer);
            int g2 = static_cast<int>(165 * shimmer);
            int b2 = static_cast<int>(32 * shimmer);

            QString style = QString(
                                "background-color: rgba(255, 255, 255, 230); "
                                "border: 5px solid qlineargradient(x1:0, y1:0, x2:1, y2:1, "
                                "stop:0 rgb(%1, %2, %3), stop:0.5 rgb(255, 223, 0), stop:1 rgb(%4, %5, %6)); "
                                "border-radius: 10px;"
                                ).arg(r1).arg(g1).arg(b1).arg(r2).arg(g2).arg(b2);

            starterBoxes[i]->setStyleSheet(style);
        } else {
            starterBoxes[i]->setStyleSheet("background-color: rgba(255, 255, 255, 230); "
                                           "border: 3px solid #999; "
                                           "border-radius: 10px;");
        }
    }
}

void LabMap::selectStarter()
{
    if (selectedStarterIndex < 0 || selectedStarterIndex >= starters.size()) return;

    chosenStarter = starters[selectedStarterIndex].name;

    selectionShimmerTimer.stop();  // Stop shimmer animation

    for (int i = 0; i < 3; i++) {
        starterBoxes[i]->hide();
        starterSprites[i]->hide();
        starterNames[i]->hide();
        starterTypes[i]->hide();
    }
    starterPromptLabel->hide();

    isSelectingStarter = false;
    dialogueActive = true;
    dialogueFinished = false;
    currentPart = 0;

    nameBoxLabel->show();
    nameLabel->show();
    textBoxLabel->show();
    dialogueLabel->show();

    QStringList finalDialogue;
    finalDialogue.append("Excellent choice! " + chosenStarter + " is a fantastic Pokemon!");
    finalDialogue.append("You and " + chosenStarter + " are going to make an amazing team! "
                                                      "Remember to train hard and treat your Pokemon with love and respect.");
    finalDialogue.append("Now, the door to your adventure awaits! Step outside when you're ready, "
                         "and your journey to become the Champion begins!");

    dialogueParts = finalDialogue;
    startTyping(dialogueParts[0]);
}
void LabMap::startDialogue()
{
    dialogueActive = true;
    currentPart = 0;
    nameBoxLabel->show();
    nameLabel->show();
    textBoxLabel->show();
    dialogueLabel->show();

    if (isInitialDialogue) {
        startTyping(dialogueParts[0]);
    } else {
        startTyping(npcDialogueParts[0]);
    }
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
    if (isSelectingStarter) return;
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

        int checkX = newPos.x() + player->boundingRect().width() * player->scale() / 2;
        int checkY = newPos.y() + player->boundingRect().height() * player->scale() - 4;

        if (!isSolidPixel(checkX, checkY)) {
            if (newPos.x() >= 0 && newPos.x() <= 480 - player->boundingRect().width() * player->scale() &&
                newPos.y() >= 0 && newPos.y() <= 272 - player->boundingRect().height() * player->scale()) {
                player->setPosition(newPos);
                checkExitTransition();
                checkNPCInteraction();
            }
        }
    }
}

void LabMap::checkNPCInteraction()
{
    // Only allow NPC interaction after starter is chosen and NOT during any active dialogue
    if (chosenStarter.isEmpty() || dialogueActive || !dialogueFinished) {
        return;
    }

    QPointF pos = player->getPosition();
    int checkX = pos.x() + player->boundingRect().width() * player->scale() / 2;
    int checkY = pos.y() + player->boundingRect().height() * player->scale() - 4;

    if (isNPCPixel(checkX, checkY)) {
        isInitialDialogue = false;
        dialogueFinished = false;
        startDialogue();
    }
}

void LabMap::checkExitTransition()
{
    QPointF pos = player->getPosition();

    if (pos.y() >= 220 && pos.x() >= 200 && pos.x() <= 280) {
        if (!chosenStarter.isEmpty()) {
            gameTimer.stop();
            player->stopAnimation();
            emit exitToOverworld();
        }
    }
}

void LabMap::gameLoop()
{
    handleMovement();
}

void LabMap::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;

    if (isSelectingStarter) {
        switch (event->key()) {
        case Qt::Key_A:
        case Qt::Key_Left:
            selectedStarterIndex--;
            if (selectedStarterIndex < 0) selectedStarterIndex = starters.size() - 1;
            updateStarterDisplay();
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            selectedStarterIndex++;
            if (selectedStarterIndex >= starters.size()) selectedStarterIndex = 0;
            updateStarterDisplay();
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            selectStarter();
            break;
        }
        return;
    }

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

            QStringList &currentDialogue = isInitialDialogue ? dialogueParts : npcDialogueParts;

            if (currentPart < currentDialogue.size()) {
                startTyping(currentDialogue[currentPart]);
            } else {
                glowTimer.stop();
                dialogueActive = false;
                dialogueFinished = true;
                nameBoxLabel->hide();
                nameLabel->hide();
                textBoxLabel->hide();
                dialogueLabel->hide();
                promptLabel->hide();

                if (isInitialDialogue && chosenStarter.isEmpty()) {
                    showStarterSelection();
                } else if (isInitialDialogue) {
                    isInitialDialogue = false;
                }
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

    // Clear all pressed keys to prevent stuck movement
    keysPressed.clear();
    if (player) {
        player->stopAnimation();
    }

    setFocus();

    // Only show initial dialogue on first visit
    if (!hasShownInitialDialogue) {
        hasShownInitialDialogue = true;
        startDialogue();
    }

    // Restart game loop when returning to lab
    if (!gameTimer.isActive()) {
        gameTimer.start(16);
    }
}

void LabMap::setPlayerSpawnPosition(const QPointF &pos)
{
    if (player) {
        player->setPosition(pos);
        player->setDirection("back");  // Face away from camera when entering lab
        player->stopAnimation();
    }
}

void LabMap::updateSelectionShimmer()
{
    shimmerPhase += 0.1f;
    if (shimmerPhase > 6.28f) {  // 2 * PI
        shimmerPhase = 0.0f;
    }
    updateStarterDisplay();
}



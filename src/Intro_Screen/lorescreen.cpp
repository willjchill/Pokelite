#include "lorescreen.h"
#include <QKeyEvent>
#include <QShowEvent>
#include <QFont>

LoreScreen::LoreScreen(QWidget *parent)
    : QWidget(parent),
    currentPart(0),
    typeIndex(0),
    isTyping(false),
    glowIntensity(0.0f),
    glowIncreasing(true)
{
    setFixedSize(480, 272);
    setFocusPolicy(Qt::StrongFocus);

    bgLabel = new QLabel(this);
    bgLabel->setGeometry(0, 0, 480, 272);
    bgLabel->setPixmap(QPixmap(":/assets/intro/title/IntroBG.png"));
    bgLabel->setScaledContents(true);

    loreTextLabel = new QLabel(this);
    loreTextLabel->setGeometry(20, 80, 440, 120);
    loreTextLabel->setWordWrap(true);
    loreTextLabel->setAlignment(Qt::AlignCenter);

    QFont font;
    font.setFamily("Courier");
    font.setPointSize(11);
    font.setStyleHint(QFont::TypeWriter);
    font.setBold(true);
    loreTextLabel->setFont(font);
    loreTextLabel->setStyleSheet("color: black; background: transparent;");

    promptLabel = new QLabel(this);
    promptLabel->setGeometry(20, 220, 440, 30);
    promptLabel->setAlignment(Qt::AlignCenter);
    QFont promptFont;
    promptFont.setFamily("Courier");
    promptFont.setPointSize(10);
    promptFont.setStyleHint(QFont::TypeWriter);
    promptFont.setBold(true);
    promptLabel->setFont(promptFont);
    promptLabel->setText("▶ Press ENTER to continue ◀");
    promptLabel->setStyleSheet("background: transparent;");
    promptLabel->hide();

    loreParts.append("Welcome to the amazing world of POKEMON!\n\n"
                     "A place filled with wonder, adventure, and "
                     "incredible creatures waiting to be discovered!");

    loreParts.append("You've traveled far to reach the Bostonia region!\n\n"
                     "Your dream of becoming the ultimate Pokemon Champion "
                     "starts here. Catch 'em all, train hard, and show the world "
                     "what you're made of!\n\n"
                     "Are you ready? Let's GO!");

    connect(&typeTimer, &QTimer::timeout, this, &LoreScreen::typeNextCharacter);
    connect(&glowTimer, &QTimer::timeout, this, &LoreScreen::updatePromptGlow);
}

LoreScreen::~LoreScreen()
{
}

void LoreScreen::startTyping(const QString &text)
{
    currentFullText = text;
    currentDisplayText = "";
    typeIndex = 0;
    isTyping = true;
    promptLabel->hide();
    glowTimer.stop();
    loreTextLabel->setText("");
    typeTimer.start(30);
}

void LoreScreen::typeNextCharacter()
{
    if (typeIndex < currentFullText.length()) {
        currentDisplayText += currentFullText[typeIndex];
        loreTextLabel->setText(currentDisplayText);
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

void LoreScreen::updatePromptGlow()
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

void LoreScreen::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Space) {

        if (isTyping) {
            typeTimer.stop();
            loreTextLabel->setText(currentFullText);
            isTyping = false;
            promptLabel->show();
            glowIntensity = 0.0f;
            glowIncreasing = true;
            glowTimer.start(30);
            return;
        }

        currentPart++;

        if (currentPart < loreParts.size()) {
            startTyping(loreParts[currentPart]);
        } else {
            glowTimer.stop();
            emit loreFinished();
        }
        return;
    }

    QWidget::keyPressEvent(event);
}

void LoreScreen::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    setFocus();
    startTyping(loreParts[0]);
}

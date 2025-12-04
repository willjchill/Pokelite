#include <QApplication>
#include "../Intro_Screen/introscreen.h"
#include "../Intro_Screen/lorescreen.h"
#include "Overworld/labmap.h"
#include "window.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    IntroScreen intro;
    LoreScreen lore;
    LabMap lab;
    Window w;

    QObject::connect(&intro, &IntroScreen::introFinished,
                     [&]() {
                         intro.hide();
                         lore.show();
                     });

    QObject::connect(&lore, &LoreScreen::loreFinished,
                     [&]() {
                         lore.hide();
                         lab.show();
                     });

    QObject::connect(&lab, &LabMap::exitToOverworld,
                     [&]() {
                         lab.hide();
                         w.show();
                     });

    intro.show();

    return a.exec();
}

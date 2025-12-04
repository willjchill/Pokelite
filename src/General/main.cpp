#include <QApplication>
#include "../Intro_Screen/introscreen.h"
#include "window.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    IntroScreen intro;
    Window w;

    // When intro is finished → go to game window
    QObject::connect(&intro, &IntroScreen::introFinished,
                     [&]() {
                         intro.hide();
                         w.show();
                     });

    intro.show();
    return a.exec();
}

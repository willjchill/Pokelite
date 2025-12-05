#include <QApplication>
#include "../Intro_Screen/introscreen.h"
#include "../Intro_Screen/lorescreen.h"
#include "../Overworld/labmap.h"
#include "fadeeffect.h"
#include "window.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    IntroScreen intro;
    LoreScreen lore;
    LabMap lab;
    Window w;

    FadeEffect *introFade = new FadeEffect(&intro);
    FadeEffect *loreFade = new FadeEffect(&lore);
    FadeEffect *labFade = new FadeEffect(&lab);
    FadeEffect *worldFade = new FadeEffect(&w);

    QObject::connect(&intro, &IntroScreen::introFinished,
                     [&, introFade]() {
                         introFade->fadeOut(300);

                         static QMetaObject::Connection conn;
                         conn = QObject::connect(introFade, &FadeEffect::fadeOutFinished, [&, introFade]() {
                             QObject::disconnect(conn);

                             QPoint pos = intro.pos();
                             intro.hide();
                             lore.move(pos);
                             lore.show();
                             lore.raise();
                             lore.activateWindow();
                             loreFade->fadeIn(300);
                         });
                     });

    QObject::connect(&lore, &LoreScreen::loreFinished,
                     [&, loreFade]() {
                         loreFade->fadeOut(300);

                         static QMetaObject::Connection conn;
                         conn = QObject::connect(loreFade, &FadeEffect::fadeOutFinished, [&, loreFade]() {
                             QObject::disconnect(conn);

                             QPoint pos = lore.pos();
                             lore.hide();
                             lab.move(pos);
                             lab.show();
                             lab.raise();
                             lab.activateWindow();
                             labFade->fadeIn(300);
                         });
                     });

    QObject::connect(&lab, &LabMap::exitToOverworld,
                     [&, labFade, worldFade]() {
                         labFade->fadeOut(300);

                         static QMetaObject::Connection conn;
                         conn = QObject::connect(labFade, &FadeEffect::fadeOutFinished, [&, worldFade]() {
                             QObject::disconnect(conn);

                             QPoint pos = lab.pos();
                             lab.hide();

                             w.clearMovementState();
                             w.move(pos);
                             w.setPlayerSpawnPosition(QPointF(303, 210));
                             w.show();
                             w.raise();
                             w.activateWindow();

                             worldFade->fadeIn(300);

                             QTimer::singleShot(50, &w, [&w]() {
                                 w.setFocus();
                                 w.activateWindow();
                             });
                         });
                     });

    QObject::connect(&w, &Window::returnToLab,
                     [&, worldFade, labFade]() {
                         worldFade->fadeOut(300);

                         static QMetaObject::Connection conn;
                         conn = QObject::connect(worldFade, &FadeEffect::fadeOutFinished, [&, labFade]() {
                             QObject::disconnect(conn);

                             QPoint pos = w.pos();
                             w.hide();
                             lab.move(pos);
                             lab.setPlayerSpawnPosition(QPointF(220, 190));
                             lab.show();
                             lab.raise();
                             lab.activateWindow();
                             labFade->fadeIn(300);
                         });
                     });

    intro.show();
    introFade->fadeIn(300);

    return a.exec();
}

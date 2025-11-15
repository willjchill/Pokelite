#include <QApplication>
#include "tetrisgame.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    TetrisGame game;
    game.setWindowTitle("Tetris");
    game.show();
    
    return app.exec();
}

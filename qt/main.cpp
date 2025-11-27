#include "mainwin.hpp"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    main_window mw;
    mw.show();

    return app.exec();
}

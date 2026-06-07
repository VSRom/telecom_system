#include <QApplication>
#include "serverwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Telecom Server");

    ServerWindow window;
    window.show();

    return app.exec();
}
#include "main.h"


int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    DeviceEmulator device;
    return app.exec();
}

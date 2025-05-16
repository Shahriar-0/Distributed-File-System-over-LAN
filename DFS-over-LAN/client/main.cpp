#include <QCoreApplication>
#include "Client.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    Client client(QHostAddress("127.0.0.1"), 4000);
    return app.exec();
}
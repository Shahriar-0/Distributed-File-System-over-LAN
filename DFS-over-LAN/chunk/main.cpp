#include <QCoreApplication>
#include <QHostAddress>

#include "chunkServer.h"

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);

    QHostAddress localIp = QHostAddress::LocalHost;
    const int N = 15;
    for (int i = 0; i < N; ++i) {
        ChunkServer* srv = new ChunkServer(i, localIp, &a);
        srv->start();
    }

    return a.exec();
}

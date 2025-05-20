#include <QCoreApplication>
#include <QHostAddress>

#include "chunkServer.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("dfs-chunk");
    QCoreApplication::setApplicationVersion("1.0");

    QHostAddress localIp = QHostAddress::LocalHost;
    const int numChunks = 15;
    for (int i = 0; i < numChunks; ++i) {
        ChunkServer* srv = new ChunkServer(i, localIp, &app);
        srv->start();
    }

    return app.exec();
}

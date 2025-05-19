#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

#include "MasterServer.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("dfs-master");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Distributed File System Master Server");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(QStringList() << "p" << "port",
        "Port on which the master server will listen (default: 4000).",
        "port", "4000");

    parser.addOption(portOption);

    parser.process(app);

    bool ok;
    quint16 port = parser.value(portOption).toUShort(&ok);
    if (!ok) {
        qCritical() << "Invalid port number provided.";
        return 1;
    }

    MasterServer server;
    if (!server.startListening(QHostAddress::Any, port)) {
        qCritical() << "Failed to start master server on port" << port;
        return 1;
    }

    qInfo() << "Master server is running on port" << port;
    return app.exec();
}

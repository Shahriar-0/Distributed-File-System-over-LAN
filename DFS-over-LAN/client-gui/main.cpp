#include <qqmlcontext.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "client.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("dfs-client-gui");
    QCoreApplication::setApplicationVersion("1.0");

    QQmlApplicationEngine engine;

    Client client(QHostAddress("127.0.0.1"), 4000);
    engine.rootContext()->setContextProperty("client", &client);
    engine.load(QUrl(QStringLiteral("qrc:/client-gui/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

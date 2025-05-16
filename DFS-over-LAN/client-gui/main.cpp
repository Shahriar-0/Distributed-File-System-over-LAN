#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <qqmlcontext.h>

#include "client.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    Client client(QHostAddress("127.0.0.1"), 4000);
    engine.rootContext()->setContextProperty("client", &client);
    engine.load(QUrl(QStringLiteral("qrc:/client-gui/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

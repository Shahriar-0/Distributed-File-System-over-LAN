#ifndef CLIENT_H
#define CLIENT_H

#include <QHostAddress>
#include <QObject>
#include <QSocketNotifier>
#include <QTcpSocket>

class Client : public QObject {
    Q_OBJECT
public:
    explicit Client(const QHostAddress& serverAddress, quint16 serverPort, QObject* parent = nullptr);
    ~Client();

signals:
    void errorOccurred(const QString& errorMessage);

private slots:
    void onStdinReady();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* m_socket;
    QSocketNotifier* m_stdinNotifier;
    QHostAddress m_serverAddress;
    quint16 m_serverPort;

    void processCommand(const QString& command);
};

#endif // CLIENT_H
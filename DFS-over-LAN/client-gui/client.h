#ifndef CLIENT_H
#define CLIENT_H

#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>

class Client : public QObject {
    Q_OBJECT
public:
    explicit Client(const QHostAddress& serverAddress, quint16 serverPort, QObject* parent = nullptr);
    ~Client();

    Q_INVOKABLE void sendCommand(const QString& command, const QString& params);

signals:
    void responseReceived(const QString& response);
    void errorOccurred(const QString& error);
    void connectionStateChanged(bool connected);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* m_socket;
    QHostAddress m_serverAddress;
    quint16 m_serverPort;
    bool m_connected = false;
};

#endif // CLIENT_H

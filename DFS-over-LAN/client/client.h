#ifndef CLIENT_H
#define CLIENT_H

#include <QHostAddress>
#include <QObject>
#include <QSocketNotifier>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QFile>

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
    void onUdpReadyRead();

private:
    QTcpSocket* m_socket;
    QSocketNotifier* m_stdinNotifier;
    QHostAddress m_serverAddress;
    quint16 m_serverPort;

    QUdpSocket* m_udpSocket;
    QFile        m_file;
    QString      m_fileId;
    qint64       m_fileSize = 0;
    int          m_numChunks = 0;
    int          m_currentChunk = 0;
    QHostAddress m_nextChunkIp;
    quint16      m_nextChunkPort;

    static constexpr int CHUNK_SIZE = 8 * 1024;

    void processCommand(const QString& command);
    void startChunkUpload();
    void sendCurrentChunk();
};

#endif // CLIENT_H

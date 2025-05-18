#ifndef CLIENT_H
#define CLIENT_H

#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QFile>
#include <QFileInfo>

class Client : public QObject {
    Q_OBJECT
public:
    explicit Client(const QHostAddress& serverAddress,
                    quint16 serverPort,
                    QObject* parent = nullptr);
    ~Client();

    Q_INVOKABLE void sendCommand(const QString& command, const QString& params);
    Q_INVOKABLE void uploadFile(const QString& filePath);

signals:
    void responseReceived(const QString& response);
    void errorOccurred(const QString& error);
    void connectionStateChanged(bool connected);

    void uploadProgress(int currentChunk, int totalChunks);
    void uploadFinished(const QString& fileId);
    void chunkAckReceived(const QString& chunkId, quint16 nextPort, bool corrupted);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

    void onUdpReadyRead();

private:
    QTcpSocket*  m_socket;
    QHostAddress m_serverAddress;
    quint16      m_serverPort;
    bool         m_connected = false;

    QUdpSocket*  m_udpSocket = nullptr;
    QFile        m_file;
    QString      m_fileId;
    qint64       m_fileSize   = 0;
    int          m_numChunks  = 0;
    int          m_currentChunk = 0;
    QHostAddress m_nextChunkIp;
    quint16      m_nextChunkPort = 0;

    static constexpr int CHUNK_SIZE = 8 * 1024;

    void startChunkUpload();
    void sendCurrentChunk();
};

#endif // CLIENT_H

#ifndef CLIENT_H
#define CLIENT_H

#include <QFile>
#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QVector>

struct ChunkServerInfo {
    QString chunkId;
    QHostAddress ip;
    quint16 port;
};

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

    void uploadProgress(int current, int total);
    void chunkAckReceived(const QString& chunkId, const QString& nextIp, quint16 nextPort, bool corrupted);
    void uploadFinished(const QString& fileId);
    void logReceived(const QString& log);

    void downloadProgress(int current, int total);
    void chunkDataReceived(const QString& chunkId, QByteArray data, bool corrupted);
    void downloadFinished(const QString& fileId, const QString& filePath, qint64 fileSize);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError);

    void onUdpReadyRead();

private:
    void uploadFileToChunk();
    void downloadFileFromChunk();

    static QString parentDirectory();
    void getChunkInfos(QString pkt, QVector<ChunkServerInfo>& infos, int idx, int numChunks);

    QTcpSocket* m_tcp;
    QUdpSocket* m_udp;
    QHostAddress m_masterIp;
    quint16 m_masterPort;
    bool m_connected = false;

    QFile m_file;
    QString m_fileId;
    qint64 m_fileSize = 0;
    int m_numChunks = 0;
    int m_currentChunk = 0;
    QVector<ChunkServerInfo> m_uploadChunks;

    QFile m_outFile;
    QString m_downloadId;
    int m_downloadChunks = 0;
    int m_downloadCurrent = 0;
    QVector<ChunkServerInfo> m_downloadChunksInfo;

    static constexpr int CHUNK_SIZE = 8 * 1024;
    static constexpr int CHUNK_SERVER_BASE_PORT = 5000;
};

#endif // CLIENT_H

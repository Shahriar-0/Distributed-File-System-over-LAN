#ifndef MASTERSERVER_H
#define MASTERSERVER_H

#include <QDebug>
#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QSet>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>

struct ChunkServerInfo {
    QString ip;
    quint16 port;

    bool operator==(const ChunkServerInfo& other) const {
        return ip == other.ip && port == other.port;
    }
};

struct ChunkInfo {
    QString chunkId;
    QVector<ChunkServerInfo> locations;
};

struct FileMetadata {
    QString fileName;
    QVector<ChunkInfo> chunks;
};

class MasterServer : public QTcpServer {
    Q_OBJECT
public:
    explicit MasterServer(QObject* parent = nullptr);
    bool startListening(const QHostAddress& address, quint16 port);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    static constexpr int NUM_CHUNK_SERVERS = 15;
    static constexpr int CHUNK_SERVER_BASE_PORT = 5000;

    QHash<int, QList<int>> chunkServerTree;
    QVector<int> dfsOrder;
    bool dfsComputed = false;

    QSet<QTcpSocket*> m_clients;
    QHash<QString, FileMetadata> fileMetadata;

    void handleRequest(QTcpSocket* client, const QByteArray& data);

    void allocateChunks(QTcpSocket* client, const QString& fileId, qint64 size);
    void lookupFile(QTcpSocket* client, const QString& fileId);
    QString getMetadataString(const FileMetadata& metadata);
    // TODO: register chunk replica
    // void registerChunkReplica(const QString& chunkId, const QString& addr, quint16 port);

    void buildBinaryTree();
    void computeDFS(int node);
};

#endif // MASTERSERVER_H

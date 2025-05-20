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
#include <QJsonObject>
#include <QJsonArray>
#include <csignal>

struct ChunkServerInfo {
    QString ip;
    quint16 port;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["ip"] = ip;
        obj["port"] = port;
        return obj;
    }

    static ChunkServerInfo fromJson(const QJsonObject& obj) {
        ChunkServerInfo info;
        info.ip = obj["ip"].toString();
        info.port = static_cast<quint16>(obj["port"].toInt());
        return info;
    }

    bool operator==(const ChunkServerInfo& other) const {
        return ip == other.ip && port == other.port;
    }
};

struct ChunkInfo {
    QString chunkId;
    QVector<ChunkServerInfo> locations;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["chunkId"] = chunkId;
        QJsonArray locArray;
        for (const auto& loc : locations) {
            locArray.append(loc.toJson());
        }
        obj["locations"] = locArray;
        return obj;
    }

    static ChunkInfo fromJson(const QJsonObject& obj) {
        ChunkInfo info;
        info.chunkId = obj["chunkId"].toString();
        QJsonArray locArray = obj["locations"].toArray();
        for (const auto& locVal : locArray) {
            info.locations.append(ChunkServerInfo::fromJson(locVal.toObject()));
        }
        return info;
    }
};

struct FileMetadata {
    QString fileName;
    QVector<ChunkInfo> chunks;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["fileName"] = fileName;
        QJsonArray chunksArray;
        for (const auto& chunk : chunks) {
            chunksArray.append(chunk.toJson());
        }
        obj["chunks"] = chunksArray;
        return obj;
    }

    static FileMetadata fromJson(const QJsonObject& obj) {
        FileMetadata meta;
        meta.fileName = obj["fileName"].toString();
        QJsonArray chunksArray = obj["chunks"].toArray();
        for (const auto& chunkVal : chunksArray) {
            meta.chunks.append(ChunkInfo::fromJson(chunkVal.toObject()));
        }
        return meta;
    }
};

class MasterServer : public QTcpServer {
    Q_OBJECT
public:
    explicit MasterServer(QObject* parent = nullptr);
    ~MasterServer() override;
    bool startListening(const QHostAddress& address, quint16 port);

    // Static method to access the instance for signal handling
    static MasterServer* instance() { return s_instance; }

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    static constexpr int NUM_CHUNK_SERVERS = 15;
    static constexpr int CHUNK_SERVER_BASE_PORT = 5000;
    static constexpr int CHUNK_SIZE = 8 * 1024;

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

    void saveLog();
    static MasterServer* s_instance; // Static instance for signal handler
    static void handleSigInt(int sig);
};

#endif // MASTERSERVER_H
#ifndef MASTERSERVER_H
#define MASTERSERVER_H

#include <QHash>
#include <QList>
#include <QSet>
#include <QTcpServer>
#include <QTcpSocket>

using FileID = QString;
using ChunkID = QString;

struct ChunkServerInfo {
    QString ip;
    quint16 port;
};

struct ChunkInfo {
    ChunkID chunkId;
    QList<ChunkServerInfo> locations; // for replication and fault tolerance
};

struct FileMetadata {
    FileID fileName;
    QList<ChunkInfo> chunks;
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
    QSet<QTcpSocket*> m_clients;
    QHash<FileID, FileMetadata> fileMetadata;

    void handleRequest(QTcpSocket* client, const QByteArray& data);
    void allocateChunks(QTcpSocket* client, const QString& fileId, qint64 size);
    void lookupFile(QTcpSocket* client, const QString& fileId);
    void registerChunkReplica(const QString& chunkId, const QString& addr, quint16 port);
};

#endif // MASTERSERVER_H
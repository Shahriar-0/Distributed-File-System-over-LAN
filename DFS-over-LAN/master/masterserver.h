#ifndef MASTERSERVER_H
#define MASTERSERVER_H

#include <QList>
#include <QMap>
#include <QTcpServer>

struct ChunkServerInfo {
    QString ip;
    quint16 port;
};

struct ChunkInfo {
    QString chunkId;
    QList<ChunkServerInfo> locations; // for replication and fault tolerance and balancing and shits like that
};

struct FileMetadata {
    QString fileName;
    QList<ChunkInfo> chunks;
};

class MasterServer : public QTcpServer {
    Q_OBJECT
public:
    explicit MasterServer(QObject* parent = nullptr);
    bool startListening(const QHostAddress& address, quint16 port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QMap<QString, FileMetadata> fileMetadata;
    QMap<QTcpSocket*, ChunkServerInfo> chunkServerMap;
};

#endif // MASTERSERVER_H

#ifndef CHUNKSERVER_H
#define CHUNKSERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QDir>
#include <QHostAddress>
#include <QMap>
#include <QVector>
#include <QList>

struct ChunkMetadata {
    QString chunkId;
    QHostAddress nextChunkIp;
    quint16 nextChunkPort;
    bool corrupted;
};

class ChunkServer : public QObject {
    Q_OBJECT
public:
    explicit ChunkServer(int serverId, const QHostAddress& localIp,
                         const QMap<int, QList<int>>& tree, QObject* parent = nullptr);
    void start();

private slots:
    void onReadyRead();

private:
    int serverId;
    QString storageDir;
    QUdpSocket* udpSocket;
    QHostAddress localIp;

    QMap<int, QList<int>> chunkServerTree;

    QVector<int> dfsOrder;
    QVector<int> dfsResult;

    void computeDFS(int node);

    int getNextChunkServerId(int currentId);
    QHostAddress getNextChunkServerAddress(int nextId);

    void processPacket(const QByteArray& packet, QHostAddress sender, quint16 senderPort);

    QByteArray encodeData(const QByteArray& data);
    QByteArray decodeData(const QByteArray& data, bool& corrupted);

    QByteArray applyNoiseToData(const QByteArray& data, double prob);

    void saveChunk(const QString& chunkId, const QByteArray& data, const QHostAddress& nextIp, quint16 nextPort, bool corrupted);
    bool loadChunk(const QString& chunkId, QByteArray& data, QHostAddress& nextIp, quint16& nextPort, bool& corrupted);

    void sendAck(QHostAddress receiver, quint16 port, const QString& chunkId, const QHostAddress& nextIp, quint16 nextPort, bool corrupted);
};

#endif // CHUNKSERVER_H

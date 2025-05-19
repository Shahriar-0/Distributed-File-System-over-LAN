#ifndef CHUNKSERVER_H
#define CHUNKSERVER_H

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QObject>
#include <QUdpSocket>

static constexpr quint16 BASE_CHUNK_PORT = 5000;

class ChunkServer : public QObject {
    Q_OBJECT
public:
    explicit ChunkServer(int serverId, const QHostAddress& localIp, QObject* parent = nullptr);
    void start();

private slots:
    void onReadyRead();

private:
    int serverId;
    quint16 listenPort;
    QHostAddress localIp;
    QUdpSocket* udpSocket;
    QString storageDir;

    QByteArray applyNoiseToData(const QByteArray& data, double prob);
    QByteArray encodeData(const QByteArray& data);
    QByteArray decodeData(const QByteArray& data, bool& corrupted);

    void processStore(const QString& chunkId, const QByteArray& data, QHostAddress sender, quint16 senderPort);
    void processRetrieve(const QString& chunkId, QHostAddress sender, quint16 senderPort);
};

#endif // CHUNKSERVER_H

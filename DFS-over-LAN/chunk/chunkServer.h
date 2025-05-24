#ifndef CHUNKSERVER_H
#define CHUNKSERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QDir>
#include <QFile>
#include <QByteArray>
#include <QHostAddress>

static constexpr quint16 BASE_CHUNK_PORT = 5000;

class ChunkServer : public QObject {
    Q_OBJECT
public:
    explicit ChunkServer(int serverId, const QHostAddress& localIp, QObject* parent = nullptr);
    void start();

private slots:
    void onReadyRead();

private:
    void processStore(const QString& chunkId, const QByteArray& encodedData,
                      QHostAddress sender, quint16 senderPort);
    void processRetrieve(const QString& chunkId, QHostAddress sender, quint16 senderPort);
    
    QByteArray addNoise(const QByteArray& data, double noiseRate);

    int serverId;
    quint16 listenPort;
    QHostAddress localIp;
    QUdpSocket* udpSocket;
    QString storageDir;
    
    static constexpr double NOISE_RATE = 0.01;
};

#endif // CHUNKSERVER_H

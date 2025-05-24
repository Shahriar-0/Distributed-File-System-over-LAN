#include "chunkServer.h"
#include "encodingUtils.h"

#include <QDebug>
#include <QRandomGenerator>

static constexpr int CHUNK_SIZE = 8 * 1024;

ChunkServer::ChunkServer(int serverId, const QHostAddress& localIp, QObject* parent)
    : QObject(parent), serverId(serverId), localIp(localIp) {
    listenPort = BASE_CHUNK_PORT + serverId;
    storageDir = QString("./CHUNK-%1").arg(serverId);
    QDir dir(storageDir);

    if (!dir.exists())
        dir.mkpath(".");

    udpSocket = new QUdpSocket(this);}

void ChunkServer::start() {
    if (!udpSocket->bind(QHostAddress::AnyIPv4, listenPort)) {
        qCritical() << "ChunkServer" << serverId << "failed to bind port" << listenPort;
        return;
    }
    connect(udpSocket, &QUdpSocket::readyRead, this, &ChunkServer::onReadyRead);
    qInfo() << "ChunkServer" << serverId << "listening on UDP port" << listenPort;
}

void ChunkServer::onReadyRead() {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray dg;
        dg.resize(int(udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        udpSocket->readDatagram(dg.data(), dg.size(), &sender, &senderPort);

        int nl = dg.indexOf('\n');
        if (nl < 0) {
            qWarning() << "ChunkServer" << serverId << "malformed packet, no newline";
            continue;
        }

        QString header = QString::fromUtf8(dg.constData(), nl).trimmed();
        QByteArray payload = dg.mid(nl + 1);
        QStringList parts = header.split(' ', Qt::SkipEmptyParts);

        if (parts[0] == "STORE" && parts.size() == 3) {
            QString cid = parts[1];
            int len = parts[2].toInt();
            if (payload.size() < len) {
                qWarning() << "ChunkServer" << serverId << "STORE payload too short:" << cid;
                continue;
            }
            processStore(cid, payload.left(len), sender, senderPort);
        }
        else if (parts[0] == "RETRIEVE" && parts.size() == 2) {
            processRetrieve(parts[1], sender, senderPort);
        }
        else {
            qWarning() << "ChunkServer" << serverId << "unknown command:" << header;
        }
    }
}

void ChunkServer::processStore(const QString& chunkId, const QByteArray& encodedData,
                               QHostAddress sender, quint16 senderPort) {
    // QByteArray noisyData = addNoise(encodedData, NOISE_RATE);
    QByteArray noisyData = encodedData;
    bool corrupted = false;
    // QByteArray decodedData = decodeChunk(noisyData, corrupted);
    QByteArray decodedData = noisyData;



    if (decodedData.isEmpty()) {
        corrupted = true;
    }

    QString filePath = storageDir + "/" + chunkId + ".bin";
    QFile f(filePath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(decodedData);
        f.close();
    } else {
        qWarning() << "ChunkServer" << serverId << "failed to write chunk" << chunkId;
    }

    QString ack = QString("ACK %1 %2 %3 %4\n")
                      .arg(chunkId).arg(localIp.toString())
                      .arg(listenPort).arg(corrupted ? 1 : 0);
    udpSocket->writeDatagram(ack.toUtf8(), sender, senderPort);

    qInfo() << "ChunkServer" << serverId << "stored chunk" << chunkId << (corrupted ? "(corrupted)" : "");
}

void ChunkServer::processRetrieve(const QString& chunkId, QHostAddress sender, quint16 senderPort) {
    QString filePath = storageDir + "/" + chunkId + ".bin";
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "ChunkServer" << serverId << "cannot open chunk" << chunkId;
        return;
    }
    QByteArray decodedData = f.readAll();
    f.close();

    // QByteArray encodedData = encodeChunk(decodedData);
    QByteArray encodedData = decodedData;


    QString header = QString("DATA %1 0 %2\n").arg(chunkId).arg(encodedData.size());
    QByteArray dg = header.toUtf8() + encodedData;
    udpSocket->writeDatagram(dg, sender, senderPort);

    qInfo() << "ChunkServer" << serverId << "served chunk" << chunkId;
}


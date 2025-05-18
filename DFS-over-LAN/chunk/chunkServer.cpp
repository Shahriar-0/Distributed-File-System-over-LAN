#include "chunkServer.h"
#include <QFile>
#include <QDataStream>
#include <QRandomGenerator>
#include <QDebug>

ChunkServer::ChunkServer(int serverId, const QHostAddress& localIp,
                         const QVector<int>& dfsOrder, QObject* parent)
    : QObject(parent), serverId(serverId + BASE_CHUNK_PORT), localIp(localIp), dfsOrder(dfsOrder) {
    storageDir = QString("./CHUNK-%1").arg(serverId);
    QDir dir(storageDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    udpSocket = new QUdpSocket(this);
    qInfo() << "ChunkServer" << serverId << "Initialized";
}

int ChunkServer::getNextChunkServerId(int currentId) {
    int index = dfsOrder.indexOf(currentId);
    if (index < 0 || dfsOrder.isEmpty()) {
        return -1;
    }
    int nextIndex = (index + 1) % dfsOrder.size();

    qInfo() << "Current server: " << currentId << ",next server: " << nextIndex;
    return dfsOrder[nextIndex];
}

QHostAddress ChunkServer::getNextChunkServerAddress(int nextId) {
    if (nextId == -1) {
        return QHostAddress::Null;
    }
    return localIp;
}

void ChunkServer::start() {
    if (!udpSocket->bind(QHostAddress::AnyIPv4, serverId)) {
        qCritical() << "ChunkServer" << serverId << "failed to bind port";
        return;
    }
    connect(udpSocket, &QUdpSocket::readyRead, this, &ChunkServer::onReadyRead);
    qInfo() << "ChunkServer" << serverId << "listening on UDP port" << serverId;
}

void ChunkServer::onReadyRead() {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        processPacket(datagram, sender, senderPort);
    }
}

void ChunkServer::processPacket(const QByteArray& packet, QHostAddress sender, quint16 senderPort) {
    if (packet.size() < 2) {
        qWarning() << "ChunkServer" << serverId << "received invalid packet";
        return;
    }

    QDataStream stream(packet);
    stream.setVersion(QDataStream::Qt_5_12);

    quint8 opCode;
    stream >> opCode;

    quint8 chunkIdLen;
    stream >> chunkIdLen;

    if (packet.size() < 2 + chunkIdLen) {
        qWarning() << "ChunkServer" << serverId << "packet too short for chunkId";
        return;
    }

    QByteArray chunkIdBytes = packet.mid(2, chunkIdLen);
    QString chunkId = QString::fromUtf8(chunkIdBytes);

    if (opCode == 0x01) {
        int offset = 2 + chunkIdLen;
        if (packet.size() < offset + 4 + 2 + 4) {
            qWarning() << "ChunkServer" << serverId << "store packet too short";
            return;
        }

        quint32 nextIpInt = *(reinterpret_cast<const quint32*>(packet.constData() + offset));
        QHostAddress nextIp = QHostAddress(nextIpInt);
        offset += 4;

        quint16 nextPort = quint16((quint8)packet[offset] << 8 | (quint8)packet[offset + 1]);
        offset += 2;

        quint32 dataLen = *(reinterpret_cast<const quint32*>(packet.constData() + offset));
        offset += 4;

        if (packet.size() < offset + int(dataLen)) {
            qWarning() << "ChunkServer" << serverId << "data length mismatch";
            return;
        }

        QByteArray data = packet.mid(offset, dataLen);

        data = applyNoiseToData(data, 0.01);

        bool corrupted = false;
        QByteArray decodedData = decodeData(data, corrupted);

        int nextId = getNextChunkServerId(serverId);
        QHostAddress computedNextIp = getNextChunkServerAddress(nextId);
        quint16 computedNextPort = (nextId == -1) ? 0 : quint16(nextId);

        saveChunk(chunkId, decodedData, computedNextIp, computedNextPort, corrupted);
        sendAck(sender, senderPort, chunkId, computedNextIp, computedNextPort, corrupted);

        qInfo() << "ChunkServer" << serverId << "stored chunk" << chunkId << (corrupted ? "(corrupted)" : "");
    }
    else if (opCode == 0x02) {
        QByteArray data;
        QHostAddress nextIp;
        quint16 nextPort;
        bool corrupted;

        if (!loadChunk(chunkId, data, nextIp, nextPort, corrupted)) {
            qWarning() << "ChunkServer" << serverId << "requested chunk not found:" << chunkId;
            return;
        }

        QByteArray encodedData = encodeData(data);

        QByteArray response;
        QDataStream out(&response, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_12);

        out << quint8(0x03);
        out << quint8(chunkIdLen);
        out.writeRawData(chunkIdBytes.constData(), chunkIdLen);
        quint32 ipInt = nextIp.toIPv4Address();
        out << ipInt;
        out << nextPort;
        out << quint8(corrupted ? 1 : 0);
        out << quint32(encodedData.size());
        out.writeRawData(encodedData.constData(), encodedData.size());

        udpSocket->writeDatagram(response, sender, senderPort);
        qInfo() << "ChunkServer" << serverId << "sent chunk" << chunkId << (corrupted ? "(corrupted)" : "");
    }
    else {
        qWarning() << "ChunkServer" << serverId << "received unknown opCode:" << opCode;
    }
}

QByteArray ChunkServer::applyNoiseToData(const QByteArray& data, double prob) {
    QByteArray noisyData = data;
    QRandomGenerator* gen = QRandomGenerator::global();

    for (int i = 0; i < noisyData.size(); ++i) {
        char byte = noisyData[i];
        for (int bit = 0; bit < 8; ++bit) {
            if (gen->generateDouble() < prob) {
                byte ^= (1 << bit);
            }
        }
        noisyData[i] = byte;
    }
    return noisyData;
}

QByteArray ChunkServer::decodeData(const QByteArray& data, bool& corrupted) {
    corrupted = false;
    return data;
}

QByteArray ChunkServer::encodeData(const QByteArray& data) {
    return data;
}

void ChunkServer::saveChunk(const QString& chunkId, const QByteArray& data, const QHostAddress& nextIp, quint16 nextPort, bool corrupted) {
    QString filePath = storageDir + "/" + chunkId + ".bin";
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
    }
    ChunkMetadata meta{chunkId, nextIp, nextPort, corrupted};
    storedChunks[chunkId] = meta;
}

bool ChunkServer::loadChunk(const QString& chunkId, QByteArray& data, QHostAddress& nextIp, quint16& nextPort, bool& corrupted) {
    QString filePath = storageDir + "/" + chunkId + ".bin";
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    data = file.readAll();
    file.close();

    if (!storedChunks.contains(chunkId)) {
        return false;
    }
    ChunkMetadata meta = storedChunks[chunkId];
    nextIp = meta.nextChunkIp;
    nextPort = meta.nextChunkPort;
    corrupted = meta.corrupted;
    return true;
}

void ChunkServer::sendAck(QHostAddress receiver, quint16 port, const QString& chunkId, const QHostAddress& nextIp, quint16 nextPort, bool corrupted) {
    QByteArray ack;
    QDataStream out(&ack, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);

    out << quint8(0x04);
    quint8 chunkIdLen = quint8(chunkId.size());
    out << chunkIdLen;
    out.writeRawData(chunkId.toUtf8().constData(), chunkIdLen);

    quint32 ipInt = nextIp.toIPv4Address();
    out << ipInt;
    out << nextPort;
    out << quint8(corrupted ? 1 : 0);

    udpSocket->writeDatagram(ack, receiver, port);
}

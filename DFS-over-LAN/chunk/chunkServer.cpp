#include "chunkServer.h"

#include <QDataStream>
#include <QDebug>
#include <QRandomGenerator>

#include "schifra_galois_field.hpp"
#include "schifra_galois_field_polynomial.hpp"
#include "schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra_reed_solomon_encoder.hpp"
#include "schifra_reed_solomon_decoder.hpp"
#include "schifra_reed_solomon_block.hpp"
#include "schifra_error_processes.hpp"

ChunkServer::ChunkServer(int serverId, const QHostAddress& localIp, QObject* parent)
    : QObject(parent), serverId(serverId), localIp(localIp) {
    listenPort = BASE_CHUNK_PORT + serverId;
    storageDir = QString("./CHUNK-%1").arg(serverId);
    QDir dir(storageDir);

    if (!dir.exists())
        dir.mkpath(".");

    udpSocket = new QUdpSocket(this);
}

void ChunkServer::start() {

    if (!udpSocket->bind(QHostAddress::AnyIPv4, listenPort)) { // Bind to specific IP for hole punching to enable NAT traversal
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
        else if (parts[0] == "RETRIEVE" && parts.size() == 2)
            processRetrieve(parts[1], sender, senderPort);
        else
            qWarning() << "ChunkServer" << serverId << "unknown command:" << header;
    }
}

void ChunkServer::processStore(const QString& chunkId, const QByteArray& data, QHostAddress sender, quint16 senderPort) {
    // QByteArray noisy = applyNoiseToData(data, 0.01); // TODO: add noise

    bool corrupted = false;
    // QByteArray decoded = decodeData(noisy, corrupted);  // TODO: add noise
    QByteArray decoded = decodeData(data, corrupted);

    QString filePath = storageDir + "/" + chunkId + ".bin";
    QFile f(filePath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(decoded);
        f.close();
    }
    else
        qWarning() << "ChunkServer" << serverId << "failed to write chunk" << chunkId;

    qInfo() << "ChunkServer" << serverId << "stored" << chunkId << (corrupted ? "(corrupted)" : "");

    QString ack = QString("ACK %1 %2 %3 %4\n").arg(chunkId).arg(localIp.toString()).arg(listenPort).arg(corrupted ? 1 : 0);

    udpSocket->writeDatagram(ack.toUtf8(), sender, senderPort);
}

void ChunkServer::processRetrieve(const QString& chunkId, QHostAddress sender, quint16 senderPort) {
    QString filePath = storageDir + "/" + chunkId + ".bin";
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "ChunkServer" << serverId << "cannot open chunk" << chunkId;
        return;
    }
    QByteArray data = f.readAll();
    f.close();
    bool corrupted = false;

    // should we encode here again? honestly don't know
    QByteArray encoded = encodeData(data);

    QString header = QString("DATA %1 %2 %3\n").arg(chunkId).arg(corrupted ? 1 : 0).arg(encoded.size());

    QByteArray dg = header.toUtf8() + encoded;
    udpSocket->writeDatagram(dg, sender, senderPort);

    qInfo() << "ChunkServer" << serverId << "served" << chunkId << (corrupted ? "(corrupted)" : "");
}

// TODO: add noise
// QByteArray ChunkServer::applyNoiseToData(const QByteArray& data, double prob) {
//     QByteArray out = data;
//     auto gen = QRandomGenerator::global();
//     for (int i = 0; i < out.size(); ++i) {
//         char byte = out[i];
//         for (int b = 0; b < 8; ++b) {
//             if (gen->generateDouble() < prob)
//                 byte ^= (1 << b);
//         }
//         out[i] = byte;
//     }
//     return out;
// }

// TODO: dummy functions - to be implemented ishalla (nmkh)
QByteArray ChunkServer::decodeData(const QByteArray& data, bool& corrupted) {
    corrupted = false;
    return data;
}

// TODO
QByteArray ChunkServer::encodeData(const QByteArray& data) {
    return data;
}

#include "client.h"

#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

Client::Client(const QHostAddress& serverAddress, quint16 serverPort, QObject* parent)
    : QObject(parent), m_masterIp(serverAddress), m_masterPort(serverPort) {
    m_tcp = new QTcpSocket(this);
    connect(m_tcp, &QTcpSocket::connected, this, &Client::onConnected);
    connect(m_tcp, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(m_tcp, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(m_tcp, &QTcpSocket::errorOccurred, this, &Client::onError);
    m_tcp->connectToHost(m_masterIp, m_masterPort);

    m_udp = new QUdpSocket(this);
    connect(m_udp, &QUdpSocket::readyRead, this, &Client::onUdpReadyRead);
    m_udp->bind(QHostAddress::AnyIPv4, 0);
}

Client::~Client() {
    m_tcp->deleteLater();
    m_udp->deleteLater();
}

void Client::sendCommand(const QString& command, const QString& params) {
    QString pkt_params = params;
    if (command == "ALLOCATE_CHUNKS") {
        QStringList parts = params.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 1) {
            m_fileId = parts[0];

            const QString parentPath = parentDirectory(); // FIXME: update for executable
            QString fullPath = QDir(parentPath).filePath(m_fileId);

            m_file.setFileName(fullPath);
            if (!m_file.open(QIODevice::ReadOnly)) {
                emit errorOccurred("Cannot open file: " + fullPath);
                return;
            }

            m_fileSize = m_file.size();
            m_numChunks = int((m_fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE);
            m_currentChunk = 0;
            m_uploadChunks.clear();

            pkt_params += " " + QString::number(m_fileSize);
        }
        else {
            emit errorOccurred("Invalid parameters for ALLOCATE_CHUNKS");
            return;
        }
    }

    else if (command == "LOOKUP_FILE") {
        // not sure yet
        m_downloadId = params.trimmed();
        m_downloadCurrent = 0;
        m_downloadChunksInfo.clear();

        m_outFile.setFileName(m_downloadId + ".download");
        if (!m_outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit errorOccurred("Cannot open output: " + m_outFile.fileName());
            return;
        }
    }

    QByteArray pkt = (command + " " + params + "\n").toUtf8();
    if (m_tcp->state() == QAbstractSocket::ConnectedState)
        m_tcp->write(pkt);
    else
        emit errorOccurred("Not connected to master server");
}

void Client::onConnected() {
    m_connected = true;
    emit connectionStateChanged(true);
}

void Client::onDisconnected() {
    m_connected = false;
    emit connectionStateChanged(false);
}

void Client::onError(QAbstractSocket::SocketError socketError) {
    // Q_UNUSED(socketError)
    emit errorOccurred(m_tcp->errorString());
    if (socketError == QAbstractSocket::ConnectionRefusedError)
        emit connectionStateChanged(false);
}

void Client::onReadyRead() {
    while (m_tcp->canReadLine()) {
        QString pkt = m_tcp->readLine().trimmed();
        emit responseReceived(pkt);

        auto parts = pkt.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 3 && parts[0] == "OK" && parts[1] == "Allocated") {
            int NumberOfChunks = parts[2].toInt();
            int idx = 3;
            getChunkInfos(pkt, m_uploadChunks, idx, NumberOfChunks);
            uploadFileToChunk();
        }
        else if (parts.size() >= 4 && parts[0] == "FILE_METADATA") {
            int NumberOfChunks = (parts.size() - 2) / 3;
            int idx = 2;
            m_downloadChunks = NumberOfChunks;
            getChunkInfos(pkt, m_downloadChunksInfo, idx, NumberOfChunks);
            downloadFileFromChunk();
        }
    }
}

void Client::getChunkInfos(QString pkt, QVector<ChunkServerInfo>& infos, int idx, int numChunks) {
    auto parts = pkt.split(' ', Qt::SkipEmptyParts);
    for (int i = 0; i < numChunks && idx + 2 < parts.size(); ++i) {
        // the + 2 is to ensure there are enough parts cause each part should consist of name, ip and port
        // im sorry for this shitty code but i hate this course and i don't have time to do it properly
        QString cid = parts[idx++];
        QHostAddress ip(parts[idx++]);
        quint16 port = parts[idx++].toUShort();
        infos.append({cid, ip, port});
    }
}

void Client::uploadFileToChunk() {
    if (m_uploadChunks.isEmpty())
        return;

    if (m_currentChunk >= m_uploadChunks.size()) {
        m_file.close();
        emit uploadFinished(m_fileId);
        return;
    }

    auto& info = m_uploadChunks[m_currentChunk];
    m_file.seek(qint64(m_currentChunk) * CHUNK_SIZE);
    QByteArray data = m_file.read(CHUNK_SIZE);
    // TODO: encode data and add noise

    QString header = QString("STORE %1 %2\n").arg(info.chunkId).arg(data.size());
    QByteArray pkt = header.toUtf8() + data;

    m_udp->writeDatagram(pkt, info.ip, info.port);
    emit responseReceived(QString("Sent %1 → %2:%3").arg(info.chunkId).arg(info.ip.toString()).arg(info.port));
}

void Client::downloadFileFromChunk() {
    if (m_downloadChunksInfo.isEmpty() || m_downloadCurrent >= m_downloadChunks)
        return;

    auto& info = m_downloadChunksInfo[m_downloadCurrent];
    QString header = QString("RETRIEVE %1\n").arg(info.chunkId);
    m_udp->writeDatagram(header.toUtf8(), info.ip, info.port);
    emit responseReceived(QString("Requested %1 → %2:%3").arg(info.chunkId).arg(info.ip.toString()).arg(info.port));
}

void Client::onUdpReadyRead() {
    while (m_udp->hasPendingDatagrams()) {
        QByteArray dg;
        dg.resize(int(m_udp->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        m_udp->readDatagram(dg.data(), dg.size(), &sender, &senderPort);

        int nl = dg.indexOf('\n');
        if (nl < 0)
            continue;
        QString hdr = QString::fromUtf8(dg.constData(), nl).trimmed();
        QByteArray payload = dg.mid(nl + 1);

        auto parts = hdr.split(' ', Qt::SkipEmptyParts);
        if (parts[0] == "ACK" && parts.size() >= 4) {
            QString cid = parts[1];
            QString nip = parts[2];
            quint16 npt = parts[3].toUShort();
            bool corrupt = (parts.size() > 4 && parts[4] == "1");
            emit chunkAckReceived(cid, nip, npt, corrupt);
            emit uploadProgress(m_currentChunk + 1, m_uploadChunks.size());
            ++m_currentChunk;
            uploadFileToChunk();
        }
        else if (parts[0] == "DATA" && parts.size() >= 4) {
            QString cid = parts[1];
            bool corrupt = (parts[2] == "1");
            int len = parts[3].toInt();
            QByteArray data = payload.left(len);
            // TODO: decode data
            m_outFile.write(data);
            emit chunkDataReceived(cid, data, corrupt);
            emit downloadProgress(m_downloadCurrent + 1, m_downloadChunks);
            ++m_downloadCurrent;
            if (m_downloadCurrent < m_downloadChunks)
                downloadFileFromChunk();
            else {
                m_outFile.close();
                emit downloadFinished(m_downloadId);
            }
        }
    }
}

QString Client::parentDirectory() {
    QFileInfo fi(__FILE__);
    return fi.absolutePath().left(fi.absolutePath().lastIndexOf('/'));
}

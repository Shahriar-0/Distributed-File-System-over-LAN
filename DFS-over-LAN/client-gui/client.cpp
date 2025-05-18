#include "Client.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDataStream>
#include <QTextStream>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

static const quint16 BASE_CHUNK_PORT = 50000;
static const quint8 OPCODE_STORE = 0x01;
static const quint8 OPCODE_ACK   = 0x04;

Client::Client(const QHostAddress& serverAddress, quint16 serverPort, QObject* parent)
    : QObject(parent),
      m_serverAddress(serverAddress),
      m_serverPort(serverPort)
{

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected,    this, &Client::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,    this, &Client::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred,this, &Client::onError);
    m_socket->connectToHost(m_serverAddress, m_serverPort);

    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &Client::onUdpReadyRead);
    m_udpSocket->bind(QHostAddress::AnyIPv4, 0);
}

Client::~Client() {
    m_socket->deleteLater();
    m_udpSocket->deleteLater();
}

void Client::sendCommand(const QString& command, const QString& params) {
    if (command == "ALLOCATE_CHUNKS") {
        QStringList p = params.split(' ', Qt::SkipEmptyParts);
        if (p.size() >= 2) {
            m_fileId    = p[0];
            m_fileSize  = p[1].toLongLong();
            m_numChunks = int((m_fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE);
            m_currentChunk = 0;
            
            // correct this later
            const QString basePath = 
                "C:/Tempp/";  
            QString fullPath = QDir(basePath).filePath(m_fileId);

            m_file.setFileName(fullPath);
            if (!m_file.open(QIODevice::ReadOnly)) {
                emit errorOccurred("Cannot open file: " + fullPath);
                return;
            }
        }
    }

    QByteArray line = (command + " " + params + "\n").toUtf8();
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(line);
    } else {
        emit errorOccurred("Not connected to server");
    }
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
    Q_UNUSED(socketError)
    emit errorOccurred(m_socket->errorString());
    if (socketError == QAbstractSocket::ConnectionRefusedError) {
        emit connectionStateChanged(false);
    }
}

void Client::onReadyRead() {
    while (m_socket->canReadLine()) {
        QString line = m_socket->readLine().trimmed();
        emit responseReceived(line);

        if (line.startsWith("OK Allocated")) {
            // Should get the first port from master - to be edited
            startChunkUpload();
        }
    }
}



void Client::uploadFile(const QString& filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        emit errorOccurred(QString("uploadFile: invalid path %1").arg(filePath));
        return;
    }

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("uploadFile: cannot open %1").arg(filePath));
        return;
    }

    m_fileId    = fi.fileName();
    m_fileSize  = m_file.size();
    m_numChunks = int((m_fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE);
    m_currentChunk = 0;

    m_nextChunkIp   = QHostAddress::LocalHost;
    m_nextChunkPort = 0;

    QString params = QString("%1 %2").arg(m_fileId).arg(m_fileSize);
    sendCommand("ALLOCATE_CHUNKS", params);
}


void Client::startChunkUpload() {
    if (m_numChunks <= 0) {
        emit errorOccurred("Master allocated zero chunks");
        return;
    }
    sendCurrentChunk();
}

void Client::sendCurrentChunk() {
    if (m_currentChunk >= m_numChunks) {
        m_file.close();
        emit uploadFinished(m_fileId);
        return;
    }

    m_file.seek(qint64(m_currentChunk) * CHUNK_SIZE);
    QByteArray data = m_file.read(CHUNK_SIZE);

    QByteArray pkt;
    QDataStream out(&pkt, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);

    QString chunkId = QString("%1_chunk_%2").arg(m_fileId).arg(m_currentChunk);
    out << quint8(OPCODE_STORE)
        << quint8(chunkId.size());
    out.writeRawData(chunkId.toUtf8().constData(), chunkId.size());

    out << quint32(0) << quint16(0);
    out << quint32(data.size());
    out.writeRawData(data.constData(), data.size());

    // Just for now
    quint16 port = quint16(m_currentChunk % m_numChunks);
    m_udpSocket->writeDatagram(pkt,
                               QHostAddress::LocalHost,
                               port + BASE_CHUNK_PORT);

    emit responseReceived(
        QString("Sent %1 to UDP port %2")
        .arg(chunkId).arg(port));
}

void Client::onUdpReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray dg;
        dg.resize(int(m_udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        m_udpSocket->readDatagram(dg.data(), dg.size(), &sender, &senderPort);

        QDataStream in(&dg, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_12);

        quint8 op; in >> op;
        if (op != OPCODE_ACK) {
            emit errorOccurred("Unexpected UDP opcode: " + QString::number(op));
            continue;
        }

        quint8 cidLen; in >> cidLen;
        QByteArray cid(cidLen, 0);
        in.readRawData(cid.data(), cidLen);

        quint32 nextIpInt; quint16 nextPort; quint8 corrupt;
        in >> nextIpInt >> nextPort >> corrupt;

        emit chunkAckReceived(
            QString::fromUtf8(cid),
            nextPort,
            corrupt != 0
        );
        emit uploadProgress(
            m_currentChunk + 1,
            m_numChunks
        );

        ++m_currentChunk;
        sendCurrentChunk();
    }
}

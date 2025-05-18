#include "Client.h"

#include <QCoreApplication>
#include <QTextStream>
#include <QDataStream>
#include <QFileInfo>
#include <QDebug>

static const quint8 OPCODE_STORE = 0x01;
static const quint8 OPCODE_ACK   = 0x04;

Client::Client(const QHostAddress& serverAddress, quint16 serverPort, QObject* parent)
    : QObject(parent),
      m_serverAddress(serverAddress),
      m_serverPort(serverPort)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &Client::onError);

    m_stdinNotifier = new QSocketNotifier(0, QSocketNotifier::Read, this);
    connect(m_stdinNotifier, &QSocketNotifier::activated, this, &Client::onStdinReady);

    m_socket->connectToHost(m_serverAddress, m_serverPort);

    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &Client::onUdpReadyRead);
    m_udpSocket->bind(QHostAddress::AnyIPv4, 0);
}

Client::~Client() {
    m_stdinNotifier->setEnabled(false);
    m_stdinNotifier->deleteLater();
    m_socket->deleteLater();
    m_udpSocket->deleteLater();
}

void Client::onStdinReady() {
    QTextStream in(stdin);
    if (!in.atEnd()) {
        QString command = in.readLine().trimmed();
        if (!command.isEmpty()) {
            processCommand(command);
        }
    }
}

void Client::processCommand(const QString& command) {
    if (command.startsWith("UPLOAD ")) {
        QString filePath = command.section(' ',1);
        QFileInfo info(filePath);
        if (!info.exists() || !info.isFile()) {
            qDebug() << "Invalid file path:" << filePath;
            return;
        }
        m_file.setFileName(filePath);
        if (!m_file.open(QIODevice::ReadOnly)) {
            qDebug() << "Cannot open file for upload";
            return;
        }
        m_fileId       = info.fileName();
        m_fileSize     = m_file.size();
        m_numChunks    = int((m_fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE);
        m_currentChunk = 0;
        m_nextChunkIp   = QHostAddress::LocalHost;
        m_nextChunkPort = BASE_CHUNK_PORT + 0;


        QString req = QString("ALLOCATE_CHUNKS %1 %2\n").arg(m_fileId).arg(m_fileSize);
        m_socket->write(req.toUtf8());
        qDebug() << "Requested allocation for" << m_fileId << ", size=" << m_fileSize;
        return;
    }

    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(command.toUtf8() + "\n");
    }
    else {
        qDebug() << "Not connected to server";
    }
}

void Client::onConnected() {
    qDebug() << "Connected to server";
}

void Client::onDisconnected() {
    qDebug() << "Disconnected from server";
}

void Client::onReadyRead() {
    while (m_socket->canReadLine()) {
        QString response = m_socket->readLine().trimmed();
        qDebug() << "Server response:" << response;

        if (response.startsWith("OK Allocated")) {
            qDebug() << "Master OK—starting chunk upload";
            startChunkUpload();
        }
    }
}

void Client::onError(QAbstractSocket::SocketError socketError) {
    qDebug() << "Socket error:" << m_socket->errorString();
    if (socketError == QAbstractSocket::ConnectionRefusedError) {
        qDebug() << "Connection refused. Is the server running?";
        QCoreApplication::exit(1);
    }
}

void Client::startChunkUpload() {
    if (m_numChunks == 0) {
        qDebug() << "Nothing to upload";
        return;
    }
    sendCurrentChunk();
}

void Client::sendCurrentChunk() {
    if (m_currentChunk >= m_numChunks) {
        qDebug() << "Upload complete for" << m_fileId;
        m_file.close();
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

    // placeholder next-ip/port (ignored by server; server recomputes)
    out << quint32(0) << quint16(0);

    out << quint32(data.size());
    out.writeRawData(data.constData(), data.size());

    m_udpSocket->writeDatagram(pkt, m_nextChunkIp, m_nextChunkPort);
    qDebug() << "Sent chunk" << chunkId << "to UDP port" << m_nextChunkPort;
}

void Client::onUdpReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        qint64 sz = m_udpSocket->pendingDatagramSize();
        if (sz < 2) {
            QByteArray junk; junk.resize(int(sz));
            m_udpSocket->readDatagram(junk.data(), junk.size());
            qDebug() << "Dropped undersized UDP datagram of" << sz << "bytes";
            continue;
        }

        QByteArray dg; dg.resize(int(sz));
        QHostAddress sender; quint16 senderPort;
        m_udpSocket->readDatagram(dg.data(), dg.size(), &sender, &senderPort);

        quint8 op = static_cast<quint8>(dg.at(0));
        if (op != OPCODE_ACK) {
            qDebug() << "Ignoring non-ACK datagram (opcode =" 
                     << op << "), payload =" << dg.toHex();
            continue;
        }

        QDataStream in(&dg, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_12);
        quint8 dummy; quint8 cidLen;
        in >> dummy >> cidLen;
        QByteArray cid(cidLen, 0);
        in.readRawData(cid.data(), cidLen);

        quint32 nextIpInt; quint16 nextPort; quint8 corrupt;
        in >> nextIpInt >> nextPort >> corrupt;

        emit chunkAckReceived(QString::fromUtf8(cid),
                              nextPort,
                              corrupt != 0);
        emit uploadProgress(m_currentChunk + 1, m_numChunks);

        ++m_currentChunk;
        sendCurrentChunk();
    }
}


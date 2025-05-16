#include "MasterServer.h"

#include <QDebug>

MasterServer::MasterServer(QObject* parent) : QTcpServer(parent) {
    connect(this, &QTcpServer::newConnection, this, &MasterServer::onNewConnection);
}

bool MasterServer::startListening(const QHostAddress& address, quint16 port) {
    if (!listen(address, port)) {
        qDebug() << "Failed to start server:" << errorString();
        return false;
    }
    qDebug() << "Server listening on" << address.toString() << ":" << port;
    return true;
}

void MasterServer::onNewConnection() {
    while (hasPendingConnections()) {
        QTcpSocket* client = nextPendingConnection();
        m_clients.insert(client);
        connect(client, &QTcpSocket::readyRead, this, &MasterServer::onReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &MasterServer::onDisconnected);
        qDebug() << "New client connected:" << client->peerAddress().toString();
    }
}

void MasterServer::onReadyRead() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    while (client->canReadLine()) {
        QByteArray data = client->readLine().trimmed();
        qDebug() << "Received data from client:" << data;
        handleRequest(client, data);
    }
}

void MasterServer::onDisconnected() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        m_clients.remove(client);
        qDebug() << "Client disconnected:" << client->peerAddress().toString();
        client->deleteLater();
    }
}

void MasterServer::handleRequest(QTcpSocket* client, const QByteArray& data) {
    QList<QByteArray> parts = data.split(' ');
    if (parts.isEmpty()) return;

    QString command = QString::fromUtf8(parts[0]);
    if (command == "ALLOCATE_CHUNKS" && parts.size() >= 3) {
        QString fileId = QString::fromUtf8(parts[1]);
        qint64 size = parts[2].toLongLong();
        allocateChunks(client, fileId, size);
    }
    else if (command == "LOOKUP_FILE" && parts.size() >= 2) {
        QString fileId = QString::fromUtf8(parts[1]);
        lookupFile(client, fileId);
    }
    else if (command == "REGISTER_CHUNK_REPLICA" && parts.size() >= 4) {
        QString chunkId = QString::fromUtf8(parts[1]);
        QString addr = QString::fromUtf8(parts[2]);
        quint16 port = parts[3].toUShort();
        registerChunkReplica(chunkId, addr, port);
    }
    else {
        client->write("ERROR Unknown command or invalid arguments\n");
    }
}

void MasterServer::allocateChunks(QTcpSocket* client, const QString& fileId, qint64 size) {
    const qint64 chunkSize = 8 * 1024; // 8KB chunks
    int numChunks = (size + chunkSize - 1) / chunkSize;
    FileMetadata metadata;
    metadata.fileName = fileId;

    for (int i = 0; i < numChunks; ++i) {
        ChunkInfo chunk;
        chunk.chunkId = fileId + "_chunk_" + QString::number(i);
        // Dummy chunk server for now
        chunk.locations.append({"127.0.0.1", 5000});
        metadata.chunks.append(chunk);
    }

    fileMetadata[fileId] = metadata;
    client->write(("OK Allocated " + QString::number(numChunks) + " chunks\n").toUtf8());
}

void MasterServer::lookupFile(QTcpSocket* client, const QString& fileId) {
    if (fileMetadata.contains(fileId)) {
        const FileMetadata& metadata = fileMetadata[fileId];
        QString response = "FILE_METADATA " + metadata.fileName;
        for (const ChunkInfo& chunk : metadata.chunks) {
            response += " " + chunk.chunkId;
            for (const ChunkServerInfo& loc : chunk.locations) {
                response += " " + loc.ip + " " + QString::number(loc.port);
            }
        }
        response += "\n";
        client->write(response.toUtf8());
    }
    else {
        client->write("ERROR File not found\n");
    }
}

void MasterServer::registerChunkReplica(const QString& chunkId, const QString& addr, quint16 port) {
    for (auto& metadata : fileMetadata) {
        for (auto& chunk : metadata.chunks) {
            if (chunk.chunkId == chunkId) {
                ChunkServerInfo newLoc{addr, port};
                if (!chunk.locations.contains(newLoc)) {
                    chunk.locations.append(newLoc);
                    qDebug() << "Registered replica for" << chunkId << "at" << addr << ":" << port;
                }
                return;
            }
        }
    }
    qDebug() << "Chunk" << chunkId << "not found for registration";
}
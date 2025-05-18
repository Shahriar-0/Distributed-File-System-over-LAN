#include "masterserver.h"
#include <QRandomGenerator>

MasterServer::MasterServer(QObject* parent)
    : QTcpServer(parent) {
    buildBinaryTree();
    computeDFS(0);
    dfsComputed = true;
    qDebug() << "Master server DFS order:" << dfsOrder;

    connect(this, &QTcpServer::newConnection, this, &MasterServer::onNewConnection);
}

bool MasterServer::startListening(const QHostAddress& address, quint16 port) {
    if (!listen(address, port)) {
        qCritical() << "Failed to start server:" << errorString();
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
    if (!client) return;

    m_clients.remove(client);
    qDebug() << "Client disconnected:" << client->peerAddress().toString();
    client->deleteLater();
}

void MasterServer::handleRequest(QTcpSocket* client, const QByteArray& data) {
    QList<QByteArray> parts = data.split(' ');
    if (parts.isEmpty()) {
        client->write("Empty command?\n");
        return;
    }

    QString command = QString::fromUtf8(parts[0]).toUpper();

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

void MasterServer::buildBinaryTree() {
    chunkServerTree.clear();
    for (int i = 0; i < NUM_CHUNK_SERVERS; ++i) {
        QList<int> children;
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        if (left < NUM_CHUNK_SERVERS) children.append(left);
        if (right < NUM_CHUNK_SERVERS) children.append(right);
        chunkServerTree[i] = children;
    }
}


void MasterServer::computeDFS(int node) {
    for (int child : chunkServerTree[node]) {
        computeDFS(child);
    }
    dfsOrder.append(node + CHUNK_SERVER_BASE_PORT);
}

void MasterServer::allocateChunks(QTcpSocket* client, const QString& fileId, qint64 size) {
    if (!dfsComputed) {
        client->write("ERROR Server topology not initialized\n");
        return;
    }

    const qint64 chunkSize = 8 * 1024; // 8 KB
    int numChunks = int((size + chunkSize - 1) / chunkSize);

    int startIdx = QRandomGenerator::global()->bounded(dfsOrder.size());

    FileMetadata metadata;
    metadata.fileName = fileId;

    for (int i = 0; i < numChunks; ++i) {
        int dfsPos = (startIdx + i) % dfsOrder.size();
        int chunkServerId = dfsOrder[dfsPos];

        ChunkInfo chunk;
        chunk.chunkId = fileId + "_chunk_" + QString::number(i);
        chunk.locations.append(ChunkServerInfo{"127.0.0.1", quint16(chunkServerId)});
        metadata.chunks.append(chunk);
    }

    fileMetadata[fileId] = metadata;

    QString response = "OK Allocated " + QString::number(numChunks);
    for (const auto& chunk : metadata.chunks) {
        response += " " + chunk.chunkId;
        for (const auto& loc : chunk.locations) {
            response += " " + loc.ip + " " + QString::number(loc.port);
        }
    }
    response += "\n";
    client->write(response.toUtf8());

    qDebug() << "Allocated" << numChunks << "chunks for file" << fileId
             << "starting from DFS index" << startIdx
             << "chunk server" << dfsOrder[startIdx];
}

void MasterServer::lookupFile(QTcpSocket* client, const QString& fileId) {
    if (!fileMetadata.contains(fileId)) {
        client->write("ERROR File not found\n");
        return;
    }

    const FileMetadata& metadata = fileMetadata[fileId];
    QString response = "FILE_METADATA " + metadata.fileName;

    for (const auto& chunk : metadata.chunks) {
        response += " " + chunk.chunkId;
        for (const auto& loc : chunk.locations) {
            response += " " + loc.ip + " " + QString::number(loc.port);
        }
    }
    response += "\n";
    client->write(response.toUtf8());
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
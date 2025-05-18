#include <QCoreApplication>
#include <QHostAddress>
#include "ChunkServer.h"

QMap<int, QList<int>> buildBinaryTreeTopology(int maxNode) {
    QMap<int, QList<int>> tree;
    for (int i = 0; i <= maxNode; ++i) {
        QList<int> children;
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        if (left <= maxNode) children.append(left);
        if (right <= maxNode) children.append(right);
        tree[i] = children;
    }
    return tree;
}

void computeDFS(int node, const QMap<int, QList<int>>& tree, QVector<int>& dfsOrder) {

    for (int child : tree[node]) {
        computeDFS(child, tree, dfsOrder);
    }
    dfsOrder.append(node);
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    const int maxNode = 14;
    QMap<int, QList<int>> tree = buildBinaryTreeTopology(maxNode);
    QVector<int> dfsOrder;
    computeDFS(0, tree, dfsOrder);

    QHostAddress chunkServerIp("172.0.0.1");

    QList<ChunkServer*> servers;
    for (int i = 0; i <= maxNode; ++i) {
        ChunkServer* server = new ChunkServer(i, chunkServerIp, dfsOrder);
        server->start();
        servers.append(server);
    }

    return a.exec();
}

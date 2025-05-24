#include "encodingUtils.h"
#include <QRandomGenerator>


QByteArray encodeChunk(const QByteArray& data) {
    const int blockSize = 8;
    QByteArray encoded;

    int numBlocks = (data.size() + blockSize - 1) / blockSize;
    for (int i = 0; i < numBlocks; ++i) {
        int start = i * blockSize;
        int length = qMin(blockSize, data.size() - start);
        QByteArray block = data.mid(start, length);

        if (block.size() < blockSize)
            block.append(QByteArray(blockSize - block.size(), 0));

        char parity = 0;
        for (char b : block)
            parity ^= b;

        encoded.append(block);
        encoded.append(parity);
    }
    return encoded;
}

QByteArray decodeChunk(const QByteArray& encodedData, bool& corrupted) {
    corrupted = false;
    if (encodedData.size() < 1) {
        corrupted = true;
        return QByteArray();
    }

    QByteArray data = encodedData.left(encodedData.size() - 1);
    char receivedChecksum = encodedData.at(encodedData.size() - 1);

    char calcChecksum = 0;
    for (char b : data) {
        calcChecksum ^= b;
    }

    if (calcChecksum != receivedChecksum) {
        corrupted = true;
    }
    return data;
}

QByteArray addNoise(const QByteArray& data, double noiseRate) {
    QByteArray noisyData = data;
    QRandomGenerator* rand = QRandomGenerator::global();

    for (int i = 0; i < noisyData.size(); ++i) {
        for (int bit = 0; bit < 8; ++bit) {
            if (rand->generateDouble() < noiseRate) {
                noisyData[i] ^= (1 << bit);
            }
        }
    }
    return noisyData;
}

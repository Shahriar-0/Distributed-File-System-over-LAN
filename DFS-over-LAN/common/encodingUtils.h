#ifndef ENCODINGUTILS_H
#define ENCODINGUTILS_H

#include <QByteArray>

QByteArray encodeChunk(const QByteArray& data);
QByteArray decodeChunk(const QByteArray& encodedData, bool& corrupted);
QByteArray addNoise(const QByteArray& data, double noiseRate);

#endif // ENCODINGUTILS_H

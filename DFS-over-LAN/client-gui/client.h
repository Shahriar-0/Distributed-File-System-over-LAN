#ifndef CLIENT_H
#define CLIENT_H

#include <QFile>
#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QVector>

namespace schifra {
namespace galois {
    class field;
    class field_polynomial;
} // namespace galois
namespace reed_solomon {
    template <std::size_t code_length, std::size_t fec_length, std::size_t data_length>
    class encoder;
    template <std::size_t code_length, std::size_t fec_length, std::size_t data_length>
    class decoder;
} // namespace reed_solomon
} // namespace schifra

struct ChunkServerInfo {
    QString chunkId;
    QHostAddress ip;
    quint16 port;
};

class Client : public QObject {
    Q_OBJECT
public:
    explicit Client(const QHostAddress& serverAddress, quint16 serverPort, QObject* parent = nullptr);
    ~Client();

    Q_INVOKABLE void sendCommand(const QString& command, const QString& params);

signals:
    void responseReceived(const QString& response);
    void errorOccurred(const QString& error);
    void connectionStateChanged(bool connected);

    void uploadProgress(int current, int total);
    void chunkAckReceived(const QString& chunkId, const QString& nextIp, quint16 nextPort, bool corrupted);
    void uploadFinished(const QString& fileId);
    void logReceived(const QString& log);

    void downloadProgress(int current, int total);
    void chunkDataReceived(const QString& chunkId, QByteArray data, bool corrupted);
    void downloadFinished(const QString& fileId, const QString& filePath, qint64 fileSize);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError);

    void onUdpReadyRead();

private:
    void uploadFileToChunk();
    void downloadFileFromChunk();

    static QString parentDirectory();
    void getChunkInfos(QString pkt, QVector<ChunkServerInfo>& infos, int idx, int numChunks);

    QByteArray encodeChunk(const QByteArray& data);
    QByteArray decodeChunk(const QByteArray& encodedData, bool& corrupted);

    QTcpSocket* m_tcp;
    QUdpSocket* m_udp;
    QHostAddress m_masterIp;
    quint16 m_masterPort;
    bool m_connected = false;

    QFile m_file;
    QString m_fileId;
    qint64 m_fileSize = 0;
    int m_numChunks = 0;
    int m_currentChunk = 0;
    qint64 m_downloadFileSize = 0;
    QVector<ChunkServerInfo> m_uploadChunks;

    QFile m_outFile;
    QString m_downloadId;
    int m_downloadChunks = 0;
    int m_downloadCurrent = 0;
    QVector<ChunkServerInfo> m_downloadChunksInfo;

    static constexpr int CHUNK_SIZE = 8 * 1024;
    static constexpr int CHUNK_SERVER_BASE_PORT = 5000;

    static constexpr std::size_t FIELD_DESCRIPTOR = 8;
    static constexpr std::size_t GENERATOR_POLYNOMIAL_INDEX = 120;
    static constexpr std::size_t GENERATOR_POLYNOMIAL_ROOT_COUNT = 32;
    static constexpr std::size_t CODE_LENGTH = 255;
    static constexpr std::size_t FEC_LENGTH = 32;
    static constexpr std::size_t DATA_LENGTH = CODE_LENGTH - FEC_LENGTH;

    schifra::galois::field* field;
    schifra::galois::field_polynomial* generator_polynomial;
    const schifra::reed_solomon::encoder<CODE_LENGTH, FEC_LENGTH, DATA_LENGTH>* encoder;
    const schifra::reed_solomon::decoder<CODE_LENGTH, FEC_LENGTH, DATA_LENGTH>* decoder;
};

#endif // CLIENT_H
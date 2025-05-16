#include "Client.h"

Client::Client(const QHostAddress& serverAddress, quint16 serverPort, QObject* parent)
    : QObject(parent),
      m_serverAddress(serverAddress),
      m_serverPort(serverPort),
      m_connected(false) {
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &Client::onError);
    m_socket->connectToHost(m_serverAddress, m_serverPort);
}

Client::~Client() {
    m_socket->deleteLater();
}

void Client::sendCommand(const QString& command) {
    if (m_connected) {
        m_socket->write(command.toUtf8() + "\n");
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

void Client::onReadyRead() {
    while (m_socket->canReadLine()) {
        QString response = m_socket->readLine().trimmed();
        emit responseReceived(response);
    }
}

void Client::onError(QAbstractSocket::SocketError socketError) {
    emit errorOccurred(m_socket->errorString());
    if (socketError == QAbstractSocket::ConnectionRefusedError) {
        m_connected = false;
        emit connectionStateChanged(false);
    }
}

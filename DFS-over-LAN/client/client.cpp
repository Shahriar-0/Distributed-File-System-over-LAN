#include "Client.h"

#include <QCoreApplication>
#include <QTextStream>

Client::Client(const QHostAddress& serverAddress, quint16 serverPort, QObject* parent)
    : QObject(parent),
      m_serverAddress(serverAddress),
      m_serverPort(serverPort) {

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &Client::onError);

    m_stdinNotifier = new QSocketNotifier(0, QSocketNotifier::Read, this);
    connect(m_stdinNotifier, &QSocketNotifier::activated, this, &Client::onStdinReady);

    m_socket->connectToHost(m_serverAddress, m_serverPort);
}

Client::~Client() {
    m_stdinNotifier->setEnabled(false);
    m_stdinNotifier->deleteLater();
    m_socket->deleteLater();
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

void Client::onConnected() {
    qDebug() << "Connected to server";
}

void Client::onDisconnected() {
    qDebug() << "Disconnected from server";
}

void Client::onReadyRead() {
    while (m_socket->canReadLine()) {
        QString response = m_socket->readLine().trimmed();
        qDebug() << "Server response: " << response;
    }
}

void Client::onError(QAbstractSocket::SocketError socketError) {
    qDebug() << "Socket error: " << m_socket->errorString();
    if (socketError == QAbstractSocket::ConnectionRefusedError) {
        qDebug() << "Connection refused. Is the server running?";
        QCoreApplication::exit(1);
    }
}

void Client::processCommand(const QString& command) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(command.toUtf8() + "\n");
    }
    else {
        qDebug() << "Not connected to server";
    }
}
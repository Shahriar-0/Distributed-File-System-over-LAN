#ifndef MASTERSERVER_H
#define MASTERSERVER_H

#include <QObject>

class MasterServer : public QObject
{
    Q_OBJECT
public:
    explicit MasterServer(QObject *parent = nullptr);

signals:
};

#endif // MASTERSERVER_H

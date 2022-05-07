#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class ThisServer: public QObject{
    Q_OBJECT

private:
    QTcpServer* qtcpServer;
    int server_port;
    QTcpSocket* client_socket;

private:
    void Send(const int code);

public:
    explicit ThisServer(QObject *parent = 0, int port = 0);
    virtual ~ThisServer(){};

public slots:
    void slotNewConnection();
    void slotReadClient();
    void slotClientDisconnected();
};

#endif // TCP_SERVER_H

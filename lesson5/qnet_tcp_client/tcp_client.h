#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>

class Client: public QObject
{
    Q_OBJECT

public:
    explicit Client(QObject *parent = 0, int port = 0, QString host = "");
    virtual ~Client(){};

private:
    void Send(const QString& message);
    void OnResponse(QNetworkReply& reply);

private slots:
//определим слоты для обработки сигналов сокета
    void slotNewConnection();
    void slotServerDisconnected();
    void slotTimeoutWaiting();

private:
    QTcpSocket *socket;
    int server_port;
    QString host_name;
    QTimer *timer;
};

#endif // TCP_CLIENT_H

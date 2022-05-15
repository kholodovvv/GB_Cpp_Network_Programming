#ifndef SSL_SERVER_H
#define SSL_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTimer>
#include <QSslSocket>
#include <QSslError>
#include <QCoreApplication>

class ThisServer: public QTcpServer{
    Q_OBJECT

private:
    //QTcpServer* qtcpServer;
    //int server_port;
    //QTcpSocket* client_socket;
    //bool isClientAutorization;
    struct client{
        QSslSocket *ssl_socket = nullptr;
        bool isAutorization = false;
        QString login = "", password = "";
    };

    std::unique_ptr<client> ssl_client = nullptr;
    QSslCertificate sslServerCertificate;
    QVector<QPair<QString, QString>> users;
    QTimer *timer = new QTimer;
    QString ca_cert_path = QCoreApplication::applicationDirPath() + "/cert/rootCA.crt";
    QString cert_path = QCoreApplication::applicationDirPath() + "/cert/server.crt";
    QString key_path = QCoreApplication::applicationDirPath() + "/cert/server.key";
    QString users_data_path = QCoreApplication::applicationDirPath() + "/users.txt";

private:
    void Send(const int code);
    bool LoadUsersData();
    QSslCertificate Handle_certificate(const QString filename);
    QSslKey Handle_key(const QString filename);


protected:
    void incomingConnection(qintptr socketDescriptor);

public:
    explicit ThisServer(QObject *parent = 0, int port = 0);
    virtual ~ThisServer(){};

public slots:
    void slotClientDisconnected();
    void slotReadyRequest();
};

#endif // SSL_SERVER_H

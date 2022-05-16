#ifndef QSSL_CLIENT_H
#define QSSL_CLIENT_H

#include <QObject>
#include <QCoreApplication>
#include <QTimer>
#include <QSslSocket>

class Client: public QObject
{
    Q_OBJECT

public:
    explicit Client(QObject *parent = 0, QString host = "", int port = 0);
    virtual ~Client(){};

private:
    void SendMessage();
    QSslCertificate Handle_certificate(QString filename);
    QSslKey Handle_key(QString filename);

public slots:
    void OnSslHandshakeFailure(QList<QSslError>);

private slots:
//определим слоты для обработки сигналов сокета
    void slotNewConnection();
    void slotServerDisconnected();
    void slotReadResponce();
    void slotRepeat_Request();

private:
    QSslSocket *socket;
    QString host_name;
    QByteArray request;
    QTimer *timer;
    QString ca_cert_path = QCoreApplication::applicationDirPath() + "/cert/rootCA.crt";
    QString cert_path = QCoreApplication::applicationDirPath() + "/cert/client.crt";
    QString key_path = QCoreApplication::applicationDirPath() + "/cert/client.key";
};

#endif // QSSL_CLIENT_H

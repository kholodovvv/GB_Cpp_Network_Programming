#include <QDebug>
#include <QNetworkAccessManager>
#include <stdio.h>
#include "qssl_client.h"
#include <QFile>
#include <QSslKey>
#include <QSslError>
#include <QList>

Client::Client(QObject *parent, QString host, int port) : QObject(parent), host_name(host){

    socket = new QSslSocket(this);

    //Указываем корневой сертификат (используется сокетом на этапе установления связи для проверки сертификата сервера)
    /*QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.addCaCertificate(Handle_certificate(cert_path));
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(config);*/

    //Или можно добавить корневой и промежуточные сертификаты, если такие имеются
    QList<QSslCertificate> certificates;
    certificates.append(Handle_certificate(ca_cert_path));
    QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
    configuration.addCaCertificates(certificates);
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(configuration);

    //Здесь указываются сертификат клиента и ключ
    /*QSslCertificate cert = Handle_certificate(cert_path);
    socket->setLocalCertificate(cert);
    socket->setPrivateKey(Handle_key(key_path));*/

    socket->setProtocol(QSsl::TlsV1_2);

    socket->connectToHostEncrypted(host_name, port); //для соединения указываем имя хоста и порт

    qDebug() << "Connected to the server " << host_name << "\r\n";

    connect(socket, &QSslSocket::disconnected, this, &Client::slotServerDisconnected); //Разрыв соединения при отключении сервера
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(OnSslHandshakeFailure(QList<QSslError>))); //Обработка ошибок возникших при рукопожатии
    //connect(socket, &QSslSocket::encrypted, this, &Client::slotNewConnection); //<-- Сигнал encrypted появляется после успешно завершенного рукопожатия
                                                                                 //(если не возникает ошибок при проверке сертификата)

    /* Ожидаение рукопожатия от сервера - 5с. */
    if (!socket->waitForEncrypted(5000)) {
        qDebug() << socket->errorString();
        slotServerDisconnected();
    }else{
        qDebug() << "Socket is encrypted.\r\n";
        slotNewConnection(); //Отправка запроса авторизации пользователя на сервер
    }

}

void Client::OnSslHandshakeFailure(QList<QSslError>)
{
    this->socket->ignoreSslErrors(); //<--Закоментировать если не нужно пропускать ошибки возникающие при проверке сертификата
}

QSslKey Client::Handle_key(QString filename){
    QFile keyFile(filename);
    QByteArray data;

    if (!keyFile.open(QIODevice::ReadOnly))
        qDebug() << "Key file not found or corrupted";

    data = keyFile.readAll();

    QSslKey ssl_key = QSslKey(data, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);

    if(ssl_key.isNull()) {
        qDebug() << "The key has no content.";
    }

    keyFile.close();

    return ssl_key;
}

QSslCertificate Client::Handle_certificate(QString filename){
    QFile certificateFile(filename);
    QByteArray data;

    if (!certificateFile.open(QIODevice::ReadOnly))
        qDebug() << "Certificate file not found or corrupted";

    data = certificateFile.readAll();

    QSslCertificate cert = QSslCertificate(data, QSsl::Pem);

    if(cert.isNull()) {
        qDebug() << "The certificate has no content.";
    }

    certificateFile.close();

    return cert;
}


void Client::slotNewConnection(){
    QTextStream in(stdin);
    QString password, crypt_password, user_login;

    do{
        qDebug() << "Enter the login:";
        in >> user_login;
        qDebug() << "Enter the password:";
        in >> password;
    }while(password.length() < 4 || user_login.length() < 4);

    //хэширование пароля пользователя перед его передачей на сервер
    crypt_password = QString(QCryptographicHash::hash((password.toStdString().c_str()), QCryptographicHash::Md5).toHex());

    request.append("GET /authorization/login=" + user_login.toStdString() + "&password=" + crypt_password.toStdString() + " HTTP/1.1\r\n");
    request.append("Host: " + host_name.toStdString() + "\r\n");
    request.append("Accept: text/html\r\n");
    request.append("Accept-Language: ru-RU\r\n");
    request.append("Connection: Keep-Alive\r\n\r\n");

    qDebug() << "The request has been sent." << "\r\n";
    socket->write(request);

    connect(socket, &QSslSocket::readyRead, this, &Client::slotReadResponce); //Чтение сообщений от сервера

    /* Ждем ответ от сервера 5с, после отправляем запрос повторно */
    timer = new QTimer();
    timer->start(5000);
    connect(timer, &QTimer::timeout, this, &Client::slotRepeat_Request);

}

void Client::slotReadResponce(){
    if (timer->isActive())timer->stop();

    request.clear();

    QTextStream in(socket->readAll());
    QString line, code_resp;

    line = in.readLine();

    code_resp = QString(line).section(" ", 1, 1);

    switch (QString(code_resp).toInt()) {
        case 200:
            qDebug() << "Server response: Ok\r\n";
            SendMessage();
        break;

        case 401:
            qDebug() << "Server response: Unauthorized\r\n";
            slotNewConnection();
        break;

        case 400:
            qDebug() << "Server response: Bad Request\r\n";
            slotServerDisconnected();
        break;
    }

}

void Client::slotRepeat_Request(){
    socket->write(request);

    /* Ждем ответ от сервера 5с, после разрываем соединение */
    timer = new QTimer();
    timer->start(5000);
    connect(timer, &QTimer::timeout, this, &Client::slotServerDisconnected); //Разрываем соединение по таймауту
}

void Client::SendMessage(){
    QTextStream in(stdin);
    QString message;

    do{
        qDebug() << "Enter the message:";
        in >> message;

    }while(message.length() < 1);

    request.append("GET /message/" + message.toStdString() + " HTTP/1.1\r\n");
    request.append("Host: " + host_name.toStdString() + "\r\n");
    request.append("Accept: text/html\r\n");
    request.append("Accept-Language: ru-RU\r\n");
    request.append("Connection: Keep-Alive\r\n\r\n");

    qDebug() << "The message has been sent." << "\r\n";
    socket->write(request);

    /* Ждем ответ от сервера 5с, после отправляем сообщение повторно */
    timer = new QTimer();
    timer->start(5000);
    connect(timer, &QTimer::timeout, this, &Client::slotRepeat_Request);
}


void Client::slotServerDisconnected(){
    QTextStream in(stdin);
    QString answer;

    qDebug() << "The connection to the server is broken\r\n\r\n";
    socket->close();

    do{
        qDebug() << "To exit, enter exit\r\n";
        in >> answer;
    }while(answer != "exit");

    QCoreApplication::exit(0);
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QString answer;
    QTextStream in(stdin);

    if(argc < 2){
        qDebug() << "For the program to work correctly, you need the hostname and port!\r\n";

        do{
            qDebug() << "To exit, enter exit\r\n";
            in >> answer;
        }while(answer != "exit");

        QCoreApplication::exit(0);
    }

    Client client(0, argv[1], QString::fromLocal8Bit(argv[2]).toInt());

    return a.exec();
}

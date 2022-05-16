#include <QDebug>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include "ssl_server.h"
#include <string>
#include <QTimer>
#include <QFile>
#include <QSslKey>

ThisServer::ThisServer(QObject *parent, int port) : QTcpServer(parent){

    if(bool result = LoadUsersData())
        qDebug() << "customer data uploaded successfully!\r\n\r\n";

    if(!this->listen(QHostAddress::Any, port)){
        qDebug() << "Unable to start the server";
        this->close();
    }else{
        qDebug() << "The server listens to the port: 443";
    }

}

bool ThisServer::LoadUsersData(){
QFile dataFile(users_data_path);
QString parse;
QPair<QString, QString> user;

if (!dataFile.open(QIODevice::ReadOnly)){
    qDebug() << "users data file not found or corrupted";
    return false;
}

//Заполняем список пользователей для авторизации и идентификации
while(!dataFile.atEnd()){
    parse = dataFile.readLine();
    user.first = QString(parse.section(" ", 0, 0)).trimmed();
    user.second = QString(parse.section(" ", 1, 1)).trimmed();
    users.push_back(user);
}

return true;
}

void ThisServer::incomingConnection(qintptr socketDescriptor){

    std::unique_ptr<client> sslclient(new client);
    ssl_client = std::move(sslclient);

    ssl_client->ssl_socket = new QSslSocket(this);

    if(!ssl_client->ssl_socket->setSocketDescriptor(socketDescriptor)){
        qDebug() << "Failed to set socket descriptor in SSLServer";
        ssl_client->ssl_socket->close();
        return;
    }

    //Указываем корневой сертификат (используется сокетом на этапе установления связи для проверки сертификата клиента)
    /*QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.addCaCertificate(Handle_certificate(ca_cert_path));
    config.setPeerVerifyMode(QSslSocket::VerifyNone); //Отключение проверки сертификата клиента
    QSslConfiguration::setDefaultConfiguration(config);*/

    //Или можно добавить корневой и промежуточные сертификаты, если такие имеются
    QList<QSslCertificate> certificates;
    certificates.append(Handle_certificate(ca_cert_path));
    QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
    configuration.addCaCertificates(certificates);
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(configuration);


    //Указываем сертификат сервера и ключ
    QSslCertificate server_cert = Handle_certificate(cert_path);
    ssl_client->ssl_socket->setLocalCertificate(server_cert);

    ssl_client->ssl_socket->setPrivateKey(Handle_key(key_path));

    ssl_client->ssl_socket->setProtocol(QSsl::TlsV1_2); //Используемая версия TLS протокола

    connect(ssl_client->ssl_socket, &QSslSocket::sslErrors, this, &ThisServer::slotClientDisconnected); //Разрыв соединения при возникновения ошибки сокета
    connect(ssl_client->ssl_socket, &QSslSocket::disconnected, this, &ThisServer::slotClientDisconnected); //Разрыв соединения при отключении клиента
    //connect(ssl_socket, &QSslSocket::encrypted, this, &ThisServer::slotAuthorizationClient); //<-- Сигнал encrypted появляется когда сервер
                                                                                      //входит в шифрованный режим т.е. после отправки рукопожатия клиенту

    ssl_client->ssl_socket->startServerEncryption(); //Отправка рукопожатия клиенту

    /* Ожидаение рукопожатия от клиента - 5с. */
    if(ssl_client->ssl_socket->waitForEncrypted(5000)){
        qDebug() << "Socket is encrypted.";

        //Включаем чтение сообщений от клиента
        connect(ssl_client->ssl_socket, &QSslSocket::readyRead, this, &ThisServer::slotReadyRequest);
    }else{
        qDebug() << ssl_client->ssl_socket->errorString();
        slotClientDisconnected();
    }

}

QSslKey ThisServer::Handle_key(const QString filename){
    QFile keyFile(filename);
    QByteArray data;

    if (!keyFile.open(QIODevice::ReadOnly))
        qDebug() << "key file not found or corrupted";

    data = keyFile.readAll();

    QSslKey ssl_key = QSslKey(data, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);

    if(ssl_key.isNull()) {
        qDebug() << "The key has no content.";
    }

    keyFile.close();

    return ssl_key;
}

QSslCertificate ThisServer::Handle_certificate(const QString filename){
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

void ThisServer::slotReadyRequest(){

    QTextStream in(ssl_client->ssl_socket->readAll());
    QString line, task, command;

    if(timer->isActive()) timer->stop(); //Если запущен таймер, то отключаем его

    line = in.readLine();

    const QStringList tokens = QString(line).split(QRegularExpression("[ \r\n][ \r\n]*"));

    //Выводим всё содержание запроса
    qDebug() << "\nClient request:\n" << line << in.readAll() << "\r\n\r\n";

 //Парсинг заголовка запроса (принимает только GET запросы)
 if(tokens[0] == "GET"){
   task = tokens[1].section("/", 1, 1);
   command = tokens[1].section("/", 2, 2);

    //Если пришёл запрос от неавторизованного пользователя
    if(!ssl_client->isAutorization){
      if(task == "authorization"){ //Если пришёл запрос на авторизацию, то извлекаем из запроса логин и пароль клиента
        QString str = command.section("=", 1, 1);
        ssl_client->login = str.section("&", 0, 0);
        str = command.section("=", 1, 1);
        ssl_client->password = command.section("=", 2, 2);

          for(auto it = users.begin(); it != users.end(); it++){
              if(it->first == ssl_client->login && it->second == ssl_client->password){
                 ssl_client->isAutorization = true;
                 Send(200);
                 return;
              }
          }

        Send(401); //Если запрашиваемая пара логин + пароль не нашлась
       }else{
        Send(400); //Если запрос не на авторизацию от неавторизованного пользователя
      }
    //Если пришёл запрос от авторизованного пользователя
    }else{

      if(task == "message"){
          qDebug() << "Message from the client:\r\n" << command << "\r\n";
          Send(200);
      }else{
          Send(400);//Если запрос на авторизацию от авторизованного пользователя
      }
    }
 }

}

void ThisServer::Send(const int code){

    QByteArray pack;
    QString str_body_pack, body_pack_size;

    //формирование пакетов в соответствии с возвращаемыми кодами ответа
        switch (code) {

            case 200:
                pack.append("HTTP/1.1 200 Ok\r\n");
                pack.append("Server: Qt_Https_Server\r\n");
                pack.append("Content-Type: text/html;charset=utf-8\r\n");
                str_body_pack = QString::fromUtf8("<Html>\r\n<Head>\r\n<Title>Server response</Title>\r\n</Head>\r\n"
                          "<Body>\r\n<br><br>\r\n<center><h1>200 Ok</h1></center>\r\n</Body>\r\n</Html>\r\n\r\n");
                body_pack_size = "Content-Length: " + QString::number(str_body_pack.size());
                body_pack_size.append("\r\n\r\n");
                pack.append(body_pack_size.toStdString().c_str());
                pack.append(str_body_pack.toStdString().c_str());
            break;

            case 400:
                pack.append("HTTP/1.1 400 Bad Request\r\n");
                pack.append("Server: Qt_Https_Server\r\n");
                pack.append("Content-Type: text/html;charset=utf-8\r\n");
                str_body_pack = QString::fromUtf8("<Html>\r\n<Head>\r\n<Title>Server response</Title>\r\n</Head>\r\n"
                              "<Body>\r\n<br><br>\r\n<center><h1>400 Bad Request</h1></center>\r\n</Body>\r\n</Html>\r\n\r\n");
                body_pack_size = "Content-Length: " + QString::number(str_body_pack.size());
                body_pack_size.append("\r\n\r\n");
                pack.append(body_pack_size.toStdString().c_str());
                pack.append(str_body_pack.toStdString().c_str());
            break;

            case 401:
                pack.append("HTTP/1.1 401 Unauthorized\r\n");
                pack.append("Server: Qt_Https_Server\r\n");
                pack.append("Content-Type: text/html;charset=utf-8\r\n");
                str_body_pack = QString::fromUtf8("<Html>\r\n<Head>\r\n<Title>Server response</Title>\r\n</Head>\r\n"
                          "<Body>\r\n<br><br>\r\n<center><h1>401 Unauthorized</h1></center>\r\n</Body>\r\n</Html>\r\n\r\n");
                body_pack_size = "Content-Length: " + QString::number(str_body_pack.size());
                body_pack_size.append("\r\n\r\n");
                pack.append(body_pack_size.toStdString().c_str());
                pack.append(str_body_pack.toStdString().c_str());
            break;
        }

    ssl_client->ssl_socket->write(pack);

    /* Ждём ответ от клиента 15 сек, затем отключаем */
    timer = new QTimer();
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &ThisServer::slotClientDisconnected);
    timer->start(15000);
}

void ThisServer::slotClientDisconnected(){
    ssl_client->ssl_socket->close();
    qDebug() << "Connecting to the client " + ssl_client->login + " closed!" << "\r\n\r\n";
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ThisServer server(0, QString::fromLocal8Bit(argv[1]).toInt());

    return a.exec();
}

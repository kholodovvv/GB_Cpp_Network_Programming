#include <QCoreApplication>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include "tcp_server.h"
#include <string>
#include <QTimer>

ThisServer::ThisServer(QObject *parent, int port) : QObject(parent), server_port(port){
    qtcpServer = new QTcpServer(this);

    if(!qtcpServer->listen(QHostAddress::Any, server_port)){
        qDebug() << "Unable to start the server";
        qtcpServer->close();
    }else{
        qDebug() << "The server listens to the port: " << port;
    }

    connect(qtcpServer, &QTcpServer::newConnection, this, &ThisServer::slotNewConnection);
}

//соединение с сигналом newConnection(), который высылается при каждом присоединении нового клиента
//из этого метода мы выполняем соединения с сигналами disconnected() и readyRead()
void ThisServer::slotNewConnection(){
    client_socket = qtcpServer->nextPendingConnection(); //возвращает сокет, посредством которого можно осуществлять дальнейшую связь с клиентом

    connect(client_socket, &QTcpSocket::disconnected, this, &ThisServer::slotClientDisconnected);
    connect(client_socket, &QTcpSocket::readyRead, this, &ThisServer::slotReadClient);

}

void ThisServer::slotReadClient(){

    const QString line = QUrl::fromPercentEncoding(client_socket->readLine());
    const QStringList tokens = QString(line).split(QRegularExpression("[ \r\n][ \r\n]*"));

    //Выводим всё содержание http запроса
    qDebug() << "\nClient request:\n" << client_socket->readAll();

    QString task, command;
    //Парсинг заголовка запроса
    if(tokens[0] == "GET"){
        task = tokens[1].section("/", 1, 1);
        command = tokens[1].section("/", 2, 2);
    }

    //Если была передана комманда (exit - отключиться от сервера, shutdown - выключить сервер), то выполняем её.
    //Если клиент передал простое сообщение, то выводим его экран.
    //QString resp;
    QStringList commands, tasks;
    commands << "exit" << "shutdown";
    tasks << "command" << "message";

    switch (tasks.indexOf(task)) {
        case 0: //command
        switch (commands.indexOf(command)) {
            case 0: //exit
                Send(202);
                QTimer::singleShot(2000, this, &ThisServer::slotClientDisconnected);
                qDebug() << "Client socket closed";
            break;

            case 1: //shutdown
                Send(202);
                QTimer::singleShot(1000, this, &ThisServer::slotClientDisconnected);
                QTimer::singleShot(2000, this, &QCoreApplication::quit);
            break;

            default:
                Send(400);
            break;
         }
        break;

        case 1: //message
            qDebug() << "\n\nMessage from the client:\n" << command << "\n"; //если клиент передал простое сообщение, то выводим его на экран
            Send(200);
        break;
    }

}

void ThisServer::Send(const int code){

    QByteArray pack;
    QString str_body_pack, body_pack_size;

    //формирование пакетов в соответствии с возвращаемыми кодами ответа
        switch (code) {
            case 202:
                pack.append("HTTP/1.1 202 Accepted\r\n");
                pack.append("Server: Qt_Http_Server\r\n");
                pack.append("Content-Type: text/html;charset=utf-8\r\n");
                str_body_pack = QString::fromUtf8("<Html>\r\n<Head>\r\n<Title>Server response</Title>\r\n</Head>\r\n"
                                  "<Body>\r\n<br><br>\r\n<center><h1>202 Accepted</h1></center>\r\n</Body>\r\n</Html>\r\n\r\n");
                body_pack_size = "Content-Length: " + QString::number(str_body_pack.size());
                body_pack_size.append("\r\n\r\n");
                pack.append(body_pack_size.toStdString().c_str());
                pack.append(str_body_pack.toStdString().c_str());

            break;

            case 200:
                pack.append("HTTP/1.1 200 Ok\r\n");
                pack.append("Server: Qt_Http_Server\r\n");
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
                pack.append("Server: Qt_Http_Server\r\n");
                pack.append("Content-Type: text/html;charset=utf-8\r\n");
                str_body_pack = QString::fromUtf8("<Html>\r\n<Head>\r\n<Title>Server response</Title>\r\n</Head>\r\n"
                              "<Body>\r\n<br><br>\r\n<center><h1>400 Bad Request</h1></center>\r\n</Body>\r\n</Html>\r\n\r\n");
                body_pack_size = "Content-Length: " + QString::number(str_body_pack.size());
                body_pack_size.append("\r\n\r\n");
                pack.append(body_pack_size.toStdString().c_str());
                pack.append(str_body_pack.toStdString().c_str());
                qDebug() << pack;
            break;
        }

client_socket->write(pack);

}

void ThisServer::slotClientDisconnected(){
    client_socket->close();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ThisServer server(0, QString::fromLocal8Bit(argv[1]).toInt());

    return a.exec();
}

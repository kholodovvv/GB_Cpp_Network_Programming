#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <stdio.h>
#include "tcp_client.h"

Client::Client(QObject *parent, int port, QString host) : QObject(parent), server_port(port), host_name(host){
    socket = new QTcpSocket(this);

    socket->connectToHost(QHostAddress(host_name), server_port);
    connect(socket, &QTcpSocket::connected, this, &Client::slotNewConnection);
    connect(socket, &QTcpSocket::disconnected, this, &Client::slotServerDisconnected);
}

void Client::slotNewConnection(){
    QTextStream in(stdin);
    QString message, answer;

    qDebug() << "Connected to the server " << host_name << "\n";

    do{
        qDebug() << "Send a message or command? (1 - command; 2 - message; 3 - exit)" << "\n";
        in >> answer;
    }while(answer.toInt() < 1 || answer.toInt() > 3);

    if(answer.toInt() == 1){

        qDebug() << "Entering a command:" << "\n";
        in >> message;

        QNetworkRequest request(QUrl("http://" + host_name + ":" + QString::number(server_port) + "/command/" + message)); //формируем запрос
        QNetworkAccessManager *amngr = new QNetworkAccessManager(this);

        amngr->get(request);

        timer = new QTimer();
        timer->setSingleShot(true);

        QEventLoop eventLoop;
        connect(timer, &QTimer::timeout, this, &Client::slotTimeoutWaiting); //Разрываем соединение по таймауту
        connect(amngr, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply) {
            if (reply->error()) { qDebug() << reply->errorString(); return; }else{Client::OnResponse(*reply);}});
        if (!timer->isActive())timer->start(5000); //Ожидаем ответ от сервера не более 5 сек.
        eventLoop.exec();

    }else if(answer.toInt() == 2){

        qDebug() << "Enter a message: ";
        in >> message;

        QNetworkRequest request(QUrl("http://" + host_name + ":" + QString::number(server_port) + "/message/" + message)); //формируем запрос
        QNetworkAccessManager *amngr = new QNetworkAccessManager(this);

        amngr->get(request);

        timer = new QTimer();
        timer->setSingleShot(true);

        QEventLoop eventLoop;
            connect(timer, &QTimer::timeout, this, &Client::slotTimeoutWaiting); //Разрываем соединение по таймауту
            connect(amngr, &QNetworkAccessManager::finished, &eventLoop, [=](QNetworkReply *reply) {
                        if (reply->error()) { qDebug() << reply->errorString(); return; }else{Client::OnResponse(*reply);}});
            if (!timer->isActive())timer->start(5000); //Ожидаем ответ от сервера не более 5 сек.
        eventLoop.exec();

    }else{

        QCoreApplication::quit();
    }

}

void Client::slotTimeoutWaiting(){
    qDebug() << "Timeout waiting for a response\n";
    if(socket->isValid()){
        Client::slotNewConnection();
    }else{
        QCoreApplication::quit();
    }
}

void Client::OnResponse(QNetworkReply &reply){
    QByteArray buffer;

    if (timer->isActive()) timer->stop();

    while(reply.bytesAvailable() > 0){
        buffer = reply.readAll();
        qDebug() << "\nServer has sended:\n" << buffer.constData();
    }

    if(socket->isValid()){
        Client::slotNewConnection();
    }else{
        QCoreApplication::quit();
    }
}

void Client::slotServerDisconnected(){
    qDebug() << "The connection to the server is broken\n";
    socket->close();
}

void Client::Send(const QString& message){
    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);

    out << message;
    memcpy(buffer.data(), message.constData(), sizeof(message)+1);
    socket->write(buffer);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Client client(0, QString::fromLocal8Bit(argv[1]).toInt(), argv[2]);

    return a.exec();
}

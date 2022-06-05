#include "chatnetwork.h"
#include <QNetworkDatagram>
#include <QTime>

ChatNetwork::ChatNetwork(QObject* parent): QObject(parent){
    send_socket = new QUdpSocket;
    listen_socket = new QUdpSocket;

    id_program = QTime::currentTime().toString("HHmmss");
}

void ChatNetwork::Mailing()
{
    //Формируем пакет с сообщением who's here
    QByteArray data;
    data.append("1/whos here/" + QString(user_name).toUtf8() + "/" + QString(id_program).toUtf8());

    QNetworkDatagram datagram;
    datagram.setDestination(QHostAddress::Broadcast, reception_port);
    datagram.setData(data);
    send_socket->writeDatagram(datagram);
}

void ChatNetwork::Listen()
{
    listen_socket->bind(QHostAddress::Any, reception_port);

    connect(listen_socket, &QUdpSocket::readyRead, this, &ChatNetwork::slotReadMessages);
}

void ChatNetwork::AddUser(QHostAddress *address, QString nikname, QString id, bool back_send)
{
    ChatUser client{};

    if(id == id_program) return; //если прилетела наша датаграмма

    if(!vecUser.empty()){
        for(auto at = vecUser.begin(); at != vecUser.end(); at++){
            if(at->program_id == id){
                return;
            }else{
                client.address = *address;
                client.nikname = nikname;
                client.program_id = id;
                listUsers.push_back(nikname + " (" + id + ")");
                vecUser.push_back(client);
            }
        }
    }else{
        client.address = *address;
        client.nikname = nikname;
        client.program_id = id;
        listUsers.push_back(nikname + " (" + id + ")");
        vecUser.push_back(client);
    }

    emit newUser();

    if(back_send)
        ChatNetwork::SendMessage(1, "hi", id_program, address);
}

void ChatNetwork::SendMessage(qint8 type, QString message, QString id, QHostAddress *address)
{
    QByteArray data;
    QNetworkDatagram datagram;

    if(type == 1){

        data.append("1/" + message.toUtf8() + "/" + user_name.toUtf8() + "/" + id.toUtf8());
        datagram.setDestination(*address, reception_port);
        datagram.setData(data);
        send_socket->writeDatagram(datagram);
    }else if(type == 2){

        data.append("2/" + message.toUtf8() + "/" + user_name.toUtf8() + "/" + id.toUtf8());
        datagram.setData(data);

        if(address == nullptr){
            datagram.setDestination(QHostAddress::Broadcast, reception_port);
            send_socket->writeDatagram(datagram);
        }else{
            datagram.setDestination(*address, reception_port);
            send_socket->writeDatagram(datagram);
        }

    }else{
        return;
    }

}

void ChatNetwork::CloseSockets()
{
    if(listen_socket->isOpen()){
        listen_socket->close();
        delete listen_socket;
    }

    if(send_socket->isOpen()){
        send_socket->close();
        delete send_socket;
    }

    emit closeConnections();
}

QString ChatNetwork::GetIdProgram()
{
    return id_program;
}

void ChatNetwork::slotReadMessages()
{
    QHostAddress *address = new QHostAddress();
    qint8 type = 0;
    QString packet, message, nikname, id;

    while(listen_socket->hasPendingDatagrams()){
        QNetworkDatagram datagram = listen_socket->receiveDatagram();
        *address = datagram.senderAddress();
        packet = datagram.data().constData();
    }

    type = QString(packet.section("/", 0, 0)).toInt();
    message = packet.section("/", 1, 1);
    nikname = packet.section("/", 2, 2);
    id = packet.section("/", 3, 3);

 //Если секции, кроме тела сообщения, не заполнены, то отбрасываем такой пакет
 if(type > 0 || nikname.length() > 0 || id.length() > 0){

    switch (type) {
        case 1:
            if(message == "whos here"){
                ChatNetwork::AddUser(address, nikname, id, true);
            }else if(message == "hi"){
                ChatNetwork::AddUser(address, nikname, id, false);
            }else if(message == "bye"){
                ChatNetwork::slotDisconnected(id);
            }

        break;

        case 2:
            if(id == id_program) return; //если прилетела наша датаграмма

            listMessages.push_back("<font color=\"blue\">Принято от " + nikname + " (" + id + "):");
            listMessages.push_back("<font color=\"black\">" + message + "</font>");

            emit newMessage();
        break;
    }


 }

}

void ChatNetwork::slotDisconnected(QString id)
{
    int count = 0;

    //ищем исключительно по id, потому что пользователей с одинаковыми именами может быть несколько
    for(auto it = vecUser.begin(); it != vecUser.end(); it++){
        count++;

        if(it->program_id == id){
            vecUser.erase(it);
            listDelUsers = listUsers.at(count);
            listUsers.removeAt(count);
            return;
        }
    }

    emit userDisconnect();
}

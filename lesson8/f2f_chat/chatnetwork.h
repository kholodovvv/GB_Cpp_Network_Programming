#ifndef CHATNETWORK_H
#define CHATNETWORK_H

#include <QUdpSocket>

class ChatNetwork: public QObject
{
    Q_OBJECT

    struct ChatUser{
        QHostAddress address;
        QString nikname;
        QString program_id;
    };

public:
   ChatNetwork(QObject* parent = nullptr);
    ~ChatNetwork(){ }

    void Mailing(); //рассылка датаграм широковещетельная, оповещение всех участников сети о нашем присутствии
    void Listen(); //прослушивание порта для приема сообщений от всех участников сети
    void AddUser(QHostAddress *address, QString nikname, QString id, bool back_send);
    void SendMessage(qint8 type, QString message, QString id, QHostAddress *address);
    void CloseSockets();
    QString GetIdProgram();

private slots:
    void slotReadMessages();
    void slotDisconnected(QString id);

signals:
    void newUser(); //испускается когда добавляется новый абонент
    void newMessage(); //испускается когда приходит новое обычное сообщение
    void userDisconnect();
    void closeConnections();

public:
    QVector<struct ChatUser> vecUser;
    QStringList listUsers, listMessages;
    QString user_name, listDelUsers;

private:
    QUdpSocket *send_socket;
    QUdpSocket *listen_socket;
    int reception_port = 8888; //порт для приёма входящих датаграм
    QString id_program; //идентификатор клиента
};



#endif // CHATNETWORK_H

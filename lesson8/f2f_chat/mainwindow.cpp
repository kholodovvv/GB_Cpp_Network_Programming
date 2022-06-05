#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->pbtnExitChat->setDisabled(true);
    ui->lneEnterMessage->setDisabled(true);
    ui->pbtnMailing->setDisabled(true);
    ui->pbtnSendMessage->setDisabled(true);

    ui->plteMessages->setReadOnly(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::AddContacts()
{
    ui->lstContacts->clear();
    ui->lstContacts->addItems(mychat->listUsers);
}

void MainWindow::DeleteContacts()
{
    AddContacts();

    ui->plteMessages->appendHtml("<font color=\"green\"><b>" + mychat->listDelUsers + " покинул чат<b></font>");
}

void MainWindow::AddMessages()
{
    for(auto it = mychat->listMessages.begin(); it != mychat->listMessages.end(); it++){
        ui->plteMessages->appendHtml(QString(it->constData()));
    }

    mychat->listMessages.clear();
}


void MainWindow::on_pbtnEnterInChat_clicked()
{
    if(!ui->lneEnterNik->text().isEmpty()){
        mychat = new ChatNetwork();

        mychat->Listen();
        mychat->user_name = ui->lneEnterNik->text();
        mychat->Mailing();

        ui->pbtnExitChat->setDisabled(false);
        ui->pbtnEnterInChat->setDisabled(true);
        ui->lneEnterNik->setDisabled(true);

        ui->lneEnterMessage->setDisabled(false);
        ui->pbtnMailing->setDisabled(false);
        ui->pbtnSendMessage->setDisabled(false);

        connect(mychat, &ChatNetwork::newUser, this, &MainWindow::AddContacts); //добавляем нового пользователя в список контактов
        connect(mychat, &ChatNetwork::newMessage, this, &MainWindow::AddMessages);
        connect(mychat, &ChatNetwork::userDisconnect, this, &MainWindow::DeleteContacts);
        connect(mychat, &ChatNetwork::closeConnections, this, &MainWindow::closeConnections);

    }else{
        QMessageBox::critical(this, "Ошибка!", "Введите свой ник!");
    }
}

void MainWindow::on_pbtnExitChat_clicked()
{
    mychat->SendMessage(1, "bye", mychat->GetIdProgram(), nullptr); //оповещение всех пользователей чата о выходе
                                                                    //почему то из-за вызова этого метода программа завершается аварийно

    ui->pbtnExitChat->setDisabled(true);
    ui->pbtnEnterInChat->setDisabled(false);
    ui->lneEnterNik->setDisabled(false);
    ui->lneEnterMessage->setDisabled(true);
    ui->pbtnMailing->setDisabled(true);
    ui->pbtnSendMessage->setDisabled(true);

    ui->lstContacts->clear();

    mychat->CloseSockets();
}

void MainWindow::on_pbtnSendMessage_clicked()
{
    if(ui->lneEnterMessage->text().isEmpty()){
        QMessageBox::critical(this, "Ошибка!", "Введите текст сообщения!");
    }else{
        int lstIndex = ui->lstContacts->currentIndex().row();
        mychat->SendMessage(2, ui->lneEnterMessage->text(), mychat->GetIdProgram(), &mychat->vecUser[lstIndex].address);

        ui->plteMessages->appendHtml("<font color=\"red\">Отправлено " + mychat->vecUser[lstIndex].nikname + ":</font>");
        ui->plteMessages->appendHtml("<font color=\"black\">" + ui->lneEnterMessage->text() + ":</font>");

        ui->lneEnterMessage->clear();
    }
}

void MainWindow::on_pbtnMailing_clicked()
{
    if(ui->lneEnterMessage->text().isEmpty()){
        QMessageBox::critical(this, "Ошибка!", "Введите текст сообщения!");
    }else{
        mychat->SendMessage(2, ui->lneEnterMessage->text(), mychat->GetIdProgram(), nullptr);

        ui->plteMessages->appendHtml("<font color=\"red\">Отправлено всем:</font>");
        ui->plteMessages->appendHtml("<font color=\"black\">" + ui->lneEnterMessage->text() + ":</font>");

        ui->lneEnterMessage->clear();
    }
}

void MainWindow::closeConnections()
{
    delete mychat;
}

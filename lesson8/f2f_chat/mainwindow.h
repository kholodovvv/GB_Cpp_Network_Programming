#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "chatnetwork.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void AddContacts();
    void DeleteContacts();
    void AddMessages();


private slots:
    void on_pbtnEnterInChat_clicked();
    void on_pbtnExitChat_clicked();
    void on_pbtnSendMessage_clicked();
    void on_pbtnMailing_clicked();
    void closeConnections();

private:
    Ui::MainWindow *ui;
    ChatNetwork *mychat;
};
#endif // MAINWINDOW_H

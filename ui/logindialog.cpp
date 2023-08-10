#include "logindialog.h"
#include "qdebug.h"
#include "ui_logindialog.h"

#include <QString>
#include <QMessageBox>

#include <src/config.h>
#include <functional>

#define Chinese(str) QString::fromLocal8Bit(str)


LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    ui->pwd_lineEdit->setEchoMode(QLineEdit::Password);

}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_login_button_clicked()
{
    // login check ignored
    // everyone can login
    auto userName = ui->user_lineEdit->text();
    if(!userName.trimmed().isEmpty())
    {
        std::hash<std::string> hashFunc;
//        Config::getInstance().setUserId(std::to_string(hashFunc(userName.toStdString()) %1000));
        Config::getInstance().set("userName", userName.toStdString());
        Config::getInstance().set("userId", userName.toStdString());
        accept(); // login success
    }else{
        QMessageBox::warning(this, Chinese("警告！"),
               Chinese("用户名或密码错误！"),
                             QMessageBox::Yes);
        ui->user_lineEdit->clear();
        ui->pwd_lineEdit->clear();

        ui->user_lineEdit->setFocus();
    }
}

void LoginDialog::on_register_button_clicked()
{
    QMessageBox::warning(this, Chinese("提示"),
           Chinese("目前无需注册，直接使用任意用户名密码登入"),
                         QMessageBox::Yes);

    ui->user_lineEdit->clear();
    ui->pwd_lineEdit->clear();

    ui->user_lineEdit->setFocus();
}


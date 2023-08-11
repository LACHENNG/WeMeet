#include "ui/chatwindow.h"
#include "ui/logindialog.h"
#include <QApplication>
#include <QFile>
#include <opus.h>
using namespace std;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // set style sheet
    QFile file(":/qss/style.qss");
    file.open(QIODevice::ReadOnly);
    QString style = file.readAll();
    file.close();
    qApp->setStyleSheet(style);


    LoginDialog login;
    if(login.exec() != QDialog::Accepted){
        return app.exec();
    }

    ChatWindow w;
    w.show();
    return app.exec();
}

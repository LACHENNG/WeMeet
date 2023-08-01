#include "ui/chatwindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ChatWindow w;
    //   w.setWindowFlags(Qt::FramelessWindowHint); //设置无边框


    w.show();

    QFile file(":/qss/style.qss");
    file.open(QIODevice::ReadOnly);
    QString style = file.readAll();
    file.close();
    qApp->setStyleSheet(style);



    return app.exec();
}

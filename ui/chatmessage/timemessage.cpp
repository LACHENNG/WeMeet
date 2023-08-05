#include "timemessage.h"
#include "ui_timemessage.h"
#include <QDateTime>
#include <QTextDocument>
#include <QTextOption>


TimeMessage::TimeMessage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::timemessage)
{
    ui->setupUi(this);
    ui->timeLabel->setText("Warn: call setTime before display");
}

void TimeMessage::setTime(QString timeSecsFromUnixEpoch)
{
    QString time = QDateTime::fromMSecsSinceEpoch(timeSecsFromUnixEpoch.toInt()).toString("hh:mm");
    ui->timeLabel->setText(time);
}

TimeMessage::~TimeMessage()
{
    delete ui;
}

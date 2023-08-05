#include "timemessage.h"
#include "ui_timemessage.h"
#include <QDateTime>
#include <QTextDocument>
#include <QTextOption>

#include <QDebug>
TimeMessage::TimeMessage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::timemessage)
{
    ui->setupUi(this);
    ui->timeLabel->setText("Warn: call setTime before display");
}

void TimeMessage::setTime(qint64 timeSecsFromUnixEpoch)
{
    QString time = QDateTime::fromSecsSinceEpoch(timeSecsFromUnixEpoch).toString("hh:mm:ss");
    ui->timeLabel->setText(time);

    setMsgTime(timeSecsFromUnixEpoch);
}

TimeMessage::~TimeMessage()
{
    delete ui;
}

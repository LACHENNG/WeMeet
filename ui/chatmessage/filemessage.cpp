#include "filemessage.h"
#include "ui_filemessage.h"
#include <QFileInfo>
#include <QFile>

FileMessage::FileMessage(TYPE msgType, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileMessage)
{
    ui->setupUi(this);
    setMessageType(msgType);
}

FileMessage::~FileMessage()
{
    delete ui;
}

void FileMessage::setTitle(const QString &title)
{
    ui->label_title->setText(title);
}

void FileMessage::setIcon(const QPixmap &fileIcon)
{

    // send file
//    QFileInfo file_info(filepath);
//    QFileIconProvider icon_provider;
//    QIcon icon = icon_provider.icon(file_info);


//    sendFileMessage(filepath);
    //QTextCursor cursor = ui->textInput->textCursor();
    //cursor.insertImage(icon.pixmap(64,64).toImage());
    //    // 创建一个QLabel对象
    //    QLabel *label = new QLabel();

    //    // 使用QIcon的pixmap()方法获取图片的QPixmap对象
    //    QPixmap pixmap = icon.pixmap(64, 64);

    //    // 使用QLabel的setPixmap()方法设置图片
    //    label->setPixmap(pixmap);
    ui->fileIcon_label->setPixmap(fileIcon);

}

void FileMessage::setFileNameStr(const QString &file_name_str)
{
    ui->filename_label->setText(file_name_str);
}

void FileMessage::setFileSize(qint64 file_size_bytes)
{
    if(file_size_bytes < 1000){ // show in `xxx B` format
        ui->filesize_label->setText(QString("%1").arg(file_size_bytes)+"B");
    }else if(file_size_bytes < 1000 * 1000){// show in `xxx KB` format
        ui->filesize_label->setText(QString("%1").arg(file_size_bytes / (1000))+"KB");
    }else if(file_size_bytes < (long long)1000 * 1000 * 1000){// show in `xxx MB` format
        ui->filesize_label->setText(QString("%1").arg(file_size_bytes/(1000*1000))+"MB");
    }else{ //// show in `xxx GB` format
        ui->filesize_label->setText(QString("%1").arg(file_size_bytes/(1000*1000*1000))+"GB");
    }
}

void FileMessage::setMessageType(TYPE type)
{
    switch (type){
    case ME:
        ui->label_title->setAlignment(Qt::AlignRight);
        ui->horizontalLayout_2->removeItem(ui->right_horizontalSpacer);
        break;
    case OTHER:
        ui->label_title->setAlignment(Qt::AlignLeft);
        ui->horizontalLayout_2->removeItem(ui->left_horizontalSpacer);
        break;
    default:
        break;
    }


}

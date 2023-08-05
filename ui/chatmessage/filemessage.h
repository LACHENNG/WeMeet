#ifndef FILEMESSAGE_H
#define FILEMESSAGE_H

#include <QWidget>
#include <ui/chatmessage/messagecommon.h>
namespace Ui {
class FileMessage;
}

// show FIle Message in format
// -------------title
// xxxxxx  filename
// xICONx
// xxxxxx  filesize
//
// in which '-' represents blank
// 'x' represents characters

// The caller should adjust it`s FileMessage widget height properly
// to show this widget nicely
// this is designed to used as a QListWidget Item
class FileMessage : public QWidget, public MessageCommon
{
    Q_OBJECT

public:
    explicit FileMessage(TYPE msgType = ME, QWidget *parent = nullptr);
    ~FileMessage();

    void setTitle(const QString& title);
    void setIcon(const QPixmap& fileIcon);
    void setFileNameStr(const QString& file_name_str);
    void setFileSize(qint64 file_size_bytes);

private:
    void setMessageType(TYPE type);

private:
    Ui::FileMessage *ui;
};

#endif // FILEMESSAGE_H

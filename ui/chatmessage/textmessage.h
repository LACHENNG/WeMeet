// Author : lang @ nwpu
// All rights reserved

#ifndef TEXTMESSAGE_H
#define TEXTMESSAGE_H

#include <QWidget>
#include <ui/chatmessage/messagecommon.h>
namespace Ui {
class TextMessage;
}


// show Text Message in format
// -------------title
// xxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxx
// xxxx
// in which '-' represents blank
// 'x' represents characters

// this TextMessage QWidget auto adjust
// its height and width
class TextMessage : public QWidget, public MessageCommon
{
    Q_OBJECT

public:
    TextMessage(TYPE type, QWidget *parent = nullptr);
    ~TextMessage();

    void setTitle(const QString& text);
    void setContent(const QString& content);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void setTextMessageType(TYPE type);

    void textContentAdjust();

private:
    Ui::TextMessage *ui;
};

#endif // TEXTMESSAGE_H

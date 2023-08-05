#include "textmessage.h"
#include "ui_textmessage.h"
#include <QDebug>

TextMessage::TextMessage(TYPE type, QWidget *parent)
  : QWidget(parent),
    ui(new Ui::TextMessage)
{
    ui->setupUi(this);
    ui->message_textEdit->setReadOnly(true);
    setTextMessageType(type);
    connect(ui->message_textEdit->document(),SIGNAL(contentsChanged()),this,SLOT(textContentAdjust()));
}

TextMessage::~TextMessage()
{
    delete ui;
}

void TextMessage::setTitle(const QString &text)
{
    ui->title_label->setText(text);
}

void TextMessage::setContent(const QString &content)
{
     ui->message_textEdit->setPlainText(content);
}

void TextMessage::resizeEvent(QResizeEvent *event)
{
     //    textContentAdjust();
}

void TextMessage::setTextMessageType(TYPE type)
{
     switch(type){
     case ME:
         ui->title_label->setAlignment(Qt::AlignRight);
         ui->horizontalLayout->removeItem(ui->right_horizontalSpacer);
         break;
     case OTHER:
         ui->title_label->setAlignment(Qt::AlignLeft);
         ui->horizontalLayout->removeItem(ui->left_horizontalSpacer);
         break;
     default:
         break;
     }
}

void TextMessage::textContentAdjust()
{

    QTextDocument *document=qobject_cast<QTextDocument*>(sender());
    document->adjustSize();
    if(document)
    {
        QTextEdit *editor=qobject_cast<QTextEdit*>(document->parent()->parent());
        if(editor)
        {
            //计算内容变化后的总面积
            int nArea = document->size().width() * document->size().height();
            //计算新的高度
            int nNewHeigh = nArea / this->width();
            //5 是修正系数
            nNewHeigh = nNewHeigh < document->size().height()
                            ? document->size().height() + 25: nNewHeigh + 25; // + 5 ?
            editor->setFixedHeight(nNewHeigh);
            this->adjustSize();
//nNewHeigh + ui->title_label->height());
        }
    }

}

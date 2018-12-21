#include "qmessagebrowser.h"

#include <QScrollBar>

QMessageBrowser::QMessageBrowser(QWidget* parent)
    : QTextBrowser (parent)
{
    setOpenLinks(false);
    setOpenExternalLinks(true);

    connect(this, SIGNAL(anchorClicked(const QUrl &)), this, SLOT(onAnchorClicked(const QUrl &)));
}

QMessageBrowser::~QMessageBrowser()
{

}

void QMessageBrowser::insertMessage(QTextCursor cursor, const QString &sender, const QString &receiver, const QDateTime &time, const QString &avatarUrl, const QString &text)
{
    QString html = "<table border='0' width='100%'><tr><td rowspan='2'><img src='%AVATAR_URL%'></td><td style='background: #0000ff;font-size: 2px;'></td></tr><tr><td width='100%' style='padding-left: 10px;'>%TEXT%</td></tr></table>";

    html.replace("%SENDER%", sender);
    html.replace("%RECEIVER%", receiver);
    html.replace("%TIME%", "11:55 PM");
    html.replace("%AVATAR_URL%", avatarUrl);
    html.replace("%TEXT%", text);

    cursor.insertHtml(html);
}

QMessageBrowser::InsertWithoutScrollingData QMessageBrowser::beginInsertWithoutScrolling()
{
    InsertWithoutScrollingData data;

    QScrollBar *scrollBar = verticalScrollBar();
    data.scrollBarMaximum = scrollBar->maximum();

    setUpdatesEnabled(false);
    data.cursor = textCursor();

    return data;
}

void QMessageBrowser::endInsertWithoutScrollig(InsertWithoutScrollingData data)
{
    setTextCursor(data.cursor);
    setUpdatesEnabled(true);

    QScrollBar *scrollBar = verticalScrollBar();
    int delta = scrollBar->maximum() - data.scrollBarMaximum;
    scrollBar->setValue(delta);
}

void QMessageBrowser::insertMoreMessagesAnchor(const QString &text)
{
    QTextCursor cursor = textCursor();
    moveCursor(QTextCursor::MoveOperation::Start);
    insertMoreMessagesAnchor(cursor, text);
}

void QMessageBrowser::insertMoreMessagesAnchor(QTextCursor cursor, const QString &text)
{
    cursor.insertHtml(QString("<a href='#more_messages'>%1</a>").arg(text));
}

void QMessageBrowser::onAnchorClicked(const QUrl &arg1)
{
    if(arg1.toString() == "#more_messages") {

        QTextCursor cursor = textCursor();

        cursor.select(QTextCursor::SelectionType::BlockUnderCursor);
        cursor.removeSelectedText();

        emit moreMessagesAnchorClicked();
    }
    // failed down
//    else {
//    //    emit QTextBrowser::anchorClicked(arg1);
//    }
}

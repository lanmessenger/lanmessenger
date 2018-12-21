#ifndef QMESSAGEBROWSER_H
#define QMESSAGEBROWSER_H

#include <QTextBrowser>

class QMessageBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    explicit QMessageBrowser(QWidget* parent = nullptr);
    virtual ~QMessageBrowser();

    void insertMoreMessagesAnchor(const QString &text);
    void insertMoreMessagesAnchor(QTextCursor cursor, const QString &text);
    void insertMessage(QTextCursor cursor, const QString &sender, const QString &receiver, const QDateTime &time, const QString &avatarUrl, const QString &text);

    typedef struct {
        QTextCursor cursor;
        int scrollBarMaximum;
    } InsertWithoutScrollingData;

    InsertWithoutScrollingData beginInsertWithoutScrolling();
    void endInsertWithoutScrollig(InsertWithoutScrollingData data);

private slots:
    void onAnchorClicked(const QUrl &arg1);

Q_SIGNALS:
    void moreMessagesAnchorClicked();
};

#endif // QMESSAGEBROWSER_H

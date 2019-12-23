#ifndef SHAREDIALOG_H
#define SHAREDIALOG_H

#include <QMainWindow>
#include <QPointer>
#include <QLabel>
#include <QMimeDatabase>
#include <QDialog>
#include <QStringListModel>

#include <functional>
#include <telegram.h>

namespace Ui {
class ShareDialog;
}

class ShareDialog : public QMainWindow
{
    Q_OBJECT

public:
    explicit ShareDialog(QWidget *parent = Q_NULLPTR);
    ~ShareDialog();

    static QString homePath();

    QString getBody() const;
    void setBody(const QString &body);

    QString getFile() const;
    void setFile(const QString &file);

    bool getHideAbout() const;
    void setHideAbout(bool hideAbout);
    void setPhone(const QString &phone);
    void setChannel(const QString &channel);
    void setSilent(bool silent);
    void setPrintDialogs(const QString &printDialogs);
    void setDialogsOnly(bool dialogsOnly);
    void setRegisterOnly(bool registerOnly);

    static qint64 generateRandomId();

private slots:
    void on_nextBtn_clicked();
    void on_sendBtn_clicked();
    void on_actionBack_triggered();
    void on_cancelBtn_clicked();
    void on_stackedWidget_currentChanged(int arg1);
    void on_actionProxy_triggered();
    void on_resetBtn_clicked();
    void on_actionAbout_triggered();
    void on_dialogBtn_clicked();

    void on_closeBtn_clicked();

    void on_deleteButton_clicked();

private:
    void initTelegram(const QString &phoneNumber);
    void initProxy();
    void doSendCode(const QString &code);
    void doCheckPassword(const QString &password);
    void getChannelDetails(const QString &name);
    void send(const InputPeer &peer, const QString &file);
    void forwardNext(const InputPeer &peer, const InputMedia &media);
    void finish();
    void getDialogs();

private:
    QString fileExists(const QString &path);
    void waitLabelHide();
    void waitLabelShow();

private:
    Ui::ShareDialog *ui;

    QStringListModel *mPhonesModel;
    Telegram *mTg;
    QMimeDatabase mMimeDb;

    User mUser;
    Chat mChat;
    QList<InputPeer> mPeers;

    QString mBody;
    QString mFile;
    QString mPhone;
    QString mChannel;
    bool mHideAbout;
    bool mSilent;
    QString mPrintDialogs;
    bool mDialogsOnly;
    bool mRegisterOnly;

    QString mCurrentSalt;
    qint64 mTotalDownloaded;
    QHash<qint64, int> mFilesTimeoutCount;
    QString mDestination;
    QVariantList mMessagesList;
    MessagesStickerSet mSticketSet;
};

#endif // SHAREDIALOG_H

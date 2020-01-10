#include "sharedialog.h"
#include "ui_sharedialog.h"
#include "ui_proxy.h"
#include "ui_about.h"
#include "ui_dialog.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMessageBox>
#include <QStringListModel>
#include <QTimer>
#include <QMimeType>
#include <QDateTime>
#include <QFileDialog>
#include <QNetworkProxy>
#include <QDesktopServices>
#include <QSettings>
#include <QJsonDocument>

#include <util/utils.h>

static QString *aseman_app_home_path = 0;

ShareDialog::ShareDialog(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ShareDialog),
    mTg(Q_NULLPTR)
{
    mHideAbout = false;
    mSilent = false;
    mDialogsOnly = false;
    mRegisterOnly = false;

    ui->setupUi(this);

    mPhonesModel = new QStringListModel(ui->phoneLine);
    mPhonesModel->setStringList( QDir(homePath()).entryList(QDir::Dirs | QDir::NoDotAndDotDot) );

    ui->phoneLine->setModel(mPhonesModel);
    ui->stackedWidget->setCurrentIndex(0);
    on_stackedWidget_currentChanged(0);

    waitLabelHide();
    initProxy();
}

void ShareDialog::initProxy()
{
    QSettings settings(homePath() + "/configs.ini", QSettings::IniFormat);

    qint32 type = settings.value("Proxy/type", 0).toInt();
    QNetworkProxy newProxy;
    switch(type)
    {
    case 0: // No Proxy
        newProxy.setType(QNetworkProxy::NoProxy);
        QNetworkProxy::setApplicationProxy(newProxy);
        break;
    case 1: // Http
        newProxy.setType(QNetworkProxy::HttpProxy);
        newProxy.setHostName( settings.value("Proxy/host").toString() );
        newProxy.setPort( settings.value("Proxy/port").toInt() );
        newProxy.setUser( settings.value("Proxy/user").toString() );
        newProxy.setPassword( settings.value("Proxy/pass").toString() );
        QNetworkProxy::setApplicationProxy(newProxy);
        break;
    case 2: // Socks5
        newProxy.setType(QNetworkProxy::Socks5Proxy);
        newProxy.setHostName( settings.value("Proxy/host").toString() );
        newProxy.setPort( settings.value("Proxy/port").toInt() );
        newProxy.setUser( settings.value("Proxy/user").toString() );
        newProxy.setPassword( settings.value("Proxy/pass").toString() );
        QNetworkProxy::setApplicationProxy(newProxy);
        break;
    }
}

void ShareDialog::on_stackedWidget_currentChanged(int)
{
    switch(ui->stackedWidget->currentIndex())
    {
    case 0: // PhoneNumber
        ui->cancelBtn->hide();
        ui->sendBtn->hide();
        ui->nextBtn->show();
        ui->resetBtn->hide();
        ui->closeBtn->hide();
        ui->toolBar->show();
        ui->actionBack->setEnabled(false);
        ui->actionProxy->setEnabled(true);
        break;

    case 1: // Code
    case 2: // Password
        show();
    [[clang::fallthrough]];

    case 3: // Channel
        ui->cancelBtn->hide();
        ui->sendBtn->hide();
        ui->nextBtn->show();
        ui->resetBtn->hide();
        ui->closeBtn->hide();
        ui->toolBar->show();
        ui->actionBack->setEnabled(true);
        ui->actionProxy->setEnabled(false);
        break;

    case 4: // Details
        ui->cancelBtn->hide();
        ui->sendBtn->show();
        ui->nextBtn->hide();
        ui->resetBtn->hide();
        ui->closeBtn->hide();
        ui->toolBar->show();
        ui->actionBack->setEnabled(true);
        ui->actionProxy->setEnabled(false);
        break;

    case 5: // Download
        ui->cancelBtn->show();
        ui->sendBtn->hide();
        ui->nextBtn->hide();
        ui->resetBtn->hide();
        ui->closeBtn->hide();
        ui->toolBar->hide();
        break;

    case 6: // Finished
        ui->cancelBtn->hide();
        ui->sendBtn->hide();
        ui->nextBtn->hide();
        ui->resetBtn->show();
        ui->closeBtn->show();
        ui->toolBar->hide();
        break;
    }
}

void ShareDialog::on_actionBack_triggered()
{
    if(ui->stackedWidget->currentIndex() == 0)
        return;

    if(ui->stackedWidget->currentIndex() == 3)
        ui->stackedWidget->setCurrentIndex(0);
    else
        ui->stackedWidget->setCurrentIndex( ui->stackedWidget->currentIndex()-1 );

    if(ui->stackedWidget->currentIndex() == 0)
    {
        mTg->deleteLater();
        mTg = Q_NULLPTR;
    }
}

void ShareDialog::on_cancelBtn_clicked()
{
    mPhone.clear();
    mChannel.clear();

    ui->stackedWidget->setCurrentIndex(4);
    initTelegram(ui->phoneLine->currentText());
}

void ShareDialog::on_deleteButton_clicked()
{
    if (ui->phoneLine->currentText().length())
        QDir(homePath() + "/" + ui->phoneLine->currentText()).removeRecursively();
    mPhonesModel->setStringList( QDir(homePath()).entryList(QDir::Dirs | QDir::NoDotAndDotDot) );
}

void ShareDialog::on_resetBtn_clicked()
{
    on_cancelBtn_clicked();
}

void ShareDialog::on_actionAbout_triggered()
{
    QDialog *about = new QDialog(this);
    Ui::About *aboutUi = new Ui::About;
    aboutUi->setupUi(about);

    connect(aboutUi->githubBtn, &QPushButton::clicked, this, [this](){
        QDesktopServices::openUrl( QUrl("https://github.com/Aseman-Land/aseman-telegram-backuper") );
    });

    about->setWindowModality(Qt::ApplicationModal);
    about->exec();

    connect(about, &QDialog::destroyed, this, [this, aboutUi](){
        delete aboutUi;
    });
    about->deleteLater();
}

void ShareDialog::on_dialogBtn_clicked()
{
    waitLabelShow();
    mTg->messagesGetDialogs(false, false, false, InputPeer::null, 100, [this](TG_MESSAGES_GET_DIALOGS_CALLBACK){
        Q_UNUSED(msgId)
        waitLabelHide();
        if(!error.null) {
            QMessageBox::critical(this, "Can't get the list", error.errorText);
            return;
        }

        QDialog *dialog = new QDialog(this);
        Ui::Dialog *dialogUi = new Ui::Dialog;
        dialogUi->setupUi(dialog);

        QVariantList dialogsList;

        QHash<qint32, User> users;
        for(const User &user: result.users())
            users[user.id()] = user;
        QHash<qint32, Chat> chats;
        for(const Chat &chat: result.chats())
            chats[chat.id()] = chat;
        for(const Dialog &dlg: result.dialogs())
        {
            QListWidgetItem *item = new QListWidgetItem();

            switch(static_cast<qint32>(dlg.peer().classType()))
            {
            case Peer::typePeerChannel:
            {
                Chat chat = chats.value(dlg.peer().channelId());
                item->setData(Qt::UserRole+1, QVariant::fromValue<Chat>(chat));
                item->setText(chat.title());
                item->setIcon(QIcon(":/icons/globe.png"));
            }
                break;
            case Peer::typePeerChat:
            {
                Chat chat = chats.value(dlg.peer().chatId());
                item->setData(Qt::UserRole+1, QVariant::fromValue<Chat>(chat));
                item->setText(chat.title());
                item->setIcon(QIcon(":/icons/user-group-new.png"));
            }
                break;
            case Peer::typePeerUser:
            {
                User user = users.value(dlg.peer().userId());
                item->setData(Qt::UserRole+2, QVariant::fromValue<User>(user));
                item->setText( (user.firstName() + " " + user.lastName()).trimmed() );
                item->setIcon(QIcon(":/icons/im-user.png"));
            }
                break;
            }

            dialogUi->listWidget->addItem(item);
        }

        connect(dialogUi->listWidget, &QListWidget::doubleClicked, dialog, &QDialog::accept);
        connect(dialog, &QDialog::accepted, this, [this, dialogUi](){
            if(dialogUi->listWidget->selectedItems().isEmpty())
                return;

            QListWidgetItem *item = dialogUi->listWidget->currentItem();

            mChat = item->data(Qt::UserRole+1).value<Chat>();
            mUser = item->data(Qt::UserRole+2).value<User>();

            if(mChat.title().count())
                ui->channelIdLine->setText( mChat.title() + ": " + QString::number(mChat.id()) );
            else
                ui->channelIdLine->setText( (mUser.firstName() + " " + mUser.lastName()).trimmed() + ": " + QString::number(mUser.id()) );

            ui->stackedWidget->setCurrentIndex(4);
        });

        dialog->setWindowModality(Qt::ApplicationModal);
        dialog->exec();

        connect(dialog, &QDialog::destroyed, this, [this, dialogUi](){
            delete dialogUi;
        });
        dialog->deleteLater();
    });
}

void ShareDialog::on_actionProxy_triggered()
{
    QDialog *proxy = new QDialog(this);
    Ui::Proxy *proxyUi = new Ui::Proxy;
    proxyUi->setupUi(proxy);

    QNetworkProxy current = QNetworkProxy::applicationProxy();
    switch( static_cast<qint32>(current.type()) )
    {
    case QNetworkProxy::NoProxy:
        proxyUi->type->setCurrentIndex(0);
        break;
    case QNetworkProxy::HttpProxy:
        proxyUi->type->setCurrentIndex(1);
        break;
    case QNetworkProxy::Socks5Proxy:
        proxyUi->type->setCurrentIndex(2);
        break;
    }
    proxyUi->host->setText(current.hostName());
    proxyUi->port->setValue(current.port());
    proxyUi->user->setText(current.user());
    proxyUi->pass->setText(current.password());

    proxy->setWindowModality(Qt::ApplicationModal);

    connect(proxy, &QDialog::accepted, this, [this, proxy, proxyUi](){
        QNetworkProxy newProxy;
        switch(proxyUi->type->currentIndex())
        {
        case 0: // No Proxy
            newProxy.setType(QNetworkProxy::NoProxy);
            QNetworkProxy::setApplicationProxy(newProxy);
            break;
        case 1: // Http
            newProxy.setType(QNetworkProxy::HttpProxy);
            newProxy.setHostName(proxyUi->host->text());
            newProxy.setPort(proxyUi->port->value());
            newProxy.setUser(proxyUi->user->text());
            newProxy.setPassword(proxyUi->pass->text());
            QNetworkProxy::setApplicationProxy(newProxy);
            break;
        case 2: // Socks5
            newProxy.setType(QNetworkProxy::Socks5Proxy);
            newProxy.setHostName(proxyUi->host->text());
            newProxy.setPort(proxyUi->port->value());
            newProxy.setUser(proxyUi->user->text());
            newProxy.setPassword(proxyUi->pass->text());
            QNetworkProxy::setApplicationProxy(newProxy);
            break;
        }

        QSettings settings(homePath() + "/configs.ini", QSettings::IniFormat);
        settings.setValue("Proxy/type", proxyUi->type->currentIndex());
        settings.setValue("Proxy/host", proxyUi->host->text());
        settings.setValue("Proxy/port", proxyUi->port->value());
        settings.setValue("Proxy/user", proxyUi->user->text());
        settings.setValue("Proxy/pass", proxyUi->pass->text());
    }, Qt::QueuedConnection);

    proxy->exec();

    connect(proxy, &QDialog::destroyed, this, [this, proxyUi](){
        delete proxyUi;
    });

    proxy->deleteLater();
}

void ShareDialog::on_nextBtn_clicked()
{
    switch(ui->stackedWidget->currentIndex())
    {
    case 0: // PhoneNumber
        initTelegram(ui->phoneLine->currentText());
        break;

    case 1: // Code
        doSendCode(ui->codeLine->text());
        break;

    case 2: // Password
        doCheckPassword(ui->passLine->text());
        break;

    case 3: // Channel
        getChannelDetails(ui->channelName->text());
        break;

    case 4: // Details
        on_sendBtn_clicked();
        break;

    case 5: // Download
        break;

    case 6: // Finished
        break;
    }
}

void ShareDialog::on_sendBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
    ui->progressBar->setValue(0);
    ui->progressLabel->setText(tr("Please wait..."));

    InputPeer peer;
    if (mPeers.count())
        peer = mPeers.takeFirst();

    if(mUser.classType() == User::typeUser)
    {
        peer.setClassType(InputPeer::typeInputPeerUser);
        peer.setAccessHash(mUser.accessHash());
        peer.setUserId(mUser.id());
    }
    else
    if(mChat.classType() == Chat::typeChat)
    {
        peer.setClassType(InputPeer::typeInputPeerChat);
        peer.setAccessHash(mChat.accessHash());
        peer.setChatId(mChat.id());
    }
    else
    if(mChat.classType() == Chat::typeChannel)
    {
        peer.setClassType(InputPeer::typeInputPeerChannel);
        peer.setAccessHash(mChat.accessHash());
        peer.setChannelId(mChat.id());
    }

    mCurrentSalt.clear();
    mTotalDownloaded = 0;
    mFilesTimeoutCount.clear();
    mMessagesList.clear();
    mDestination.clear();
    ui->sizeLabel->setText("");

    send(peer, mFile);
}

void ShareDialog::on_closeBtn_clicked()
{
    QApplication::quit();
}

void ShareDialog::initTelegram(const QString &_phoneNumber)
{
    if(mTg)
        mTg->deleteLater();

    QString phoneNumber = "+98" + _phoneNumber.right(10);

    mTg = new Telegram("149.154.167.50", 443, 2,  13682, "de37bcf00f4688de900510f4f87384bb", phoneNumber,
                      homePath() + "/",
                      ":/tg-server.pub");

    connect(mTg, &Telegram::authLoggedIn, this, [this](){
        if (mRegisterOnly)
            QCoreApplication::quit();
        getDialogs();
        if (mDialogsOnly)
            return;

        ui->stackedWidget->setCurrentIndex(3);
        waitLabelHide();
        if (mChannel.isEmpty())
            on_dialogBtn_clicked();
        else
            on_nextBtn_clicked();
    });
    connect(mTg, &Telegram::authNeeded, this, [this](){
        ui->stackedWidget->setCurrentIndex(1);
        waitLabelShow();

        mTg->authSendCode([this](TG_AUTH_SEND_CODE_CALLBACK){
            Q_UNUSED(msgId)
            waitLabelHide();
            if(!error.null) {
                QMessageBox::critical(this, "Send code error", error.errorText);
                ui->stackedWidget->setCurrentIndex(0);
                return;
            }
        });
    });

    waitLabelShow();
    mTg->init();
}

void ShareDialog::doSendCode(const QString &code)
{
    if(!mTg)
    {
        ui->stackedWidget->setCurrentIndex(0);
        return;
    }

    waitLabelShow();
    mTg->authSignIn(code, [this](TG_AUTH_SIGN_IN_CALLBACK){
        Q_UNUSED(msgId)
        if(error.errorText == "SESSION_PASSWORD_NEEDED") {
            mTg->accountGetPassword([this](TG_ACCOUNT_GET_PASSWORD_CALLBACK){
                Q_UNUSED(msgId)
                waitLabelHide();
                if(!error.null) {
                    QMessageBox::critical(this, "Get password error", error.errorText);
                    ui->stackedWidget->setCurrentIndex(0);
                    return;
                }

                //As a workaround for the binary corruption of the AccountPassword we store it here as a string, there by guaranteeing deep copy
                mCurrentSalt = QString(result.currentSalt().toHex());
                ui->stackedWidget->setCurrentIndex(2);
            });
        }
        else
        if(!error.null) {
            waitLabelHide();
            QMessageBox::critical(this, "Invalid code", error.errorText);
            ui->stackedWidget->setCurrentIndex(1);
            return;
        }
    });
}

void ShareDialog::doCheckPassword(const QString &password)
{
    waitLabelShow();
    //Reconstructing a byte array with the current salt. A better solution could be made of course
    const QByteArray salt = QByteArray::fromHex(mCurrentSalt.toUtf8());
    QByteArray passData = salt + password.toUtf8() + salt;

    //Moved hash calculation here. Either the whole hash is done here or in the lower lib. Splitting makes things hard to analyze & debug
    mTg->authCheckPassword(QCryptographicHash::hash(passData, QCryptographicHash::Sha256), [this](TG_AUTH_CHECK_PASSWORD_CALLBACK){
        Q_UNUSED(msgId)
        Q_UNUSED(result)
        waitLabelHide();
        if(!error.null) {
            QMessageBox::critical(this, "Invalid password", error.errorText);
            ui->stackedWidget->setCurrentIndex(2);
            return;
        }
    });
}

void ShareDialog::getChannelDetails(const QString &name)
{
    if(!mTg)
    {
        ui->stackedWidget->setCurrentIndex(0);
        return;
    }
    if (name.isEmpty())
    {
        ui->stackedWidget->setCurrentIndex(4);
        on_nextBtn_clicked();
        return;
    }

    mUser = User::typeUserEmpty;
    mChat = Chat::typeChatEmpty;
    mSticketSet = MessagesStickerSet();

    waitLabelShow();
    mTg->contactsResolveUsername(name, [this, name](TG_CONTACTS_RESOLVE_USERNAME_CALLBACK){
        waitLabelHide();

        if(!error.null) {
            // Maybe Its sticker?
            QString errorText = error.errorText;

            InputStickerSet input;
            input.setClassType(InputStickerSet::typeInputStickerSetShortName);
            input.setShortName(name);
            mTg->messagesGetStickerSet(input, [this, errorText](TG_MESSAGES_GET_STICKER_SET_CALLBACK){
                if(!error.null) {
                    // Maybe Its sticker?
                    QMessageBox::critical(this, "Invalid Channel", errorText);
                    ui->stackedWidget->setCurrentIndex(3);
                    return;
                }

                mSticketSet = result;
                ui->channelIdLine->setText("Its sticker: " + mSticketSet.set().title());
                ui->stackedWidget->setCurrentIndex(4);
            });
            return;
        }

        if(result.chats().count())
        {
            mChat = result.chats().first();
            ui->channelIdLine->setText( mChat.title() + ": " + QString::number(mChat.id()) );
        }
        else
        if(result.users().count())
        {
            mUser = result.users().first();
            ui->channelIdLine->setText( (mUser.firstName() + " " + mUser.lastName()).trimmed() + ": " + QString::number(mUser.id()) );
        }

        ui->stackedWidget->setCurrentIndex(4);
        if (mBody.length())
            on_nextBtn_clicked();
    });

}

void ShareDialog::send(const InputPeer &peer, const QString &file)
{
    mTg->messagesSendDocument(peer, generateRandomId(), file, "", false, 0, ReplyMarkup::null, false, false,
                              false, ui->captionTextEdit->toPlainText(), [this, peer, file](TG_UPLOAD_SEND_FILE_CUSTOM_CALLBACK){
        switch (static_cast<qint64>(result.classType()))
        {
        case UploadSendFile::typeUploadSendFileCanceled:
            qDebug() << "Canceled";
            break;

        case UploadSendFile::typeUploadSendFileProgress:
            ui->progressBar->setValue(1000.0 * result.uploaded() / result.totalSize());
            break;

        case UploadSendFile::typeUploadSendFileFinished:
        {
            if (mPeers.isEmpty())
            {
                qDebug() << "Finished";
                finish();
            }

            bool forwarding = false;
            for(const Update &upd: result.updates().updates())
            {
                if (upd.message().media().classType() == MessageMedia::typeMessageMediaDocument)
                {
                    MessageMedia med = upd.message().media();
                    Document doc = med.document();

                    InputDocument inputDoc(InputDocument::typeInputDocument);
                    inputDoc.setId(doc.id());
                    inputDoc.setAccessHash(doc.accessHash());

                    InputMedia inputMedia(InputMedia::typeInputMediaDocument);
                    inputMedia.setIdInputDocument(inputDoc);
                    inputMedia.setCaption(med.caption());

                    qDebug() << "Forwarding";
                    forwarding = true;
                    forwardNext(mPeers.takeFirst(), inputMedia);
                }
            }

            if (!forwarding)
            {
                qDebug() << "Finished";
                finish();
            }
        }
            break;
        }
    });
}

void ShareDialog::forwardNext(const InputPeer &peer, const InputMedia &inputMedia)
{
    mTg->messagesSendMedia(false, false, false, peer, false, inputMedia, generateRandomId(), ReplyMarkup::null, [this, inputMedia](TG_MESSAGES_SEND_MEDIA_CALLBACK){
        if (mPeers.isEmpty())
        {
            qDebug() << "Finished";
            finish();
        }
        else
            forwardNext(mPeers.takeFirst(), inputMedia);
    });
}

QString ShareDialog::fileExists(const QString &path)
{
    const QString &dir = path.left(path.lastIndexOf("/"));
    QString fileName = path.mid(dir.length()+1);

    int lastIdx = fileName.lastIndexOf(".");
    if(lastIdx != -1)
        fileName = fileName.left(fileName.lastIndexOf("."));

    const QStringList &files = QDir(dir).entryList(QDir::NoDotAndDotDot|QDir::Files|QDir::Dirs);
    for(const QString &f: files)
    {
        lastIdx = f.lastIndexOf(".");
        if(lastIdx == -1) {
            if(fileName == f)
                return f;
        } else {
            if(fileName == f.left(lastIdx))
                return f;
        }
    }
    return QString();
}

void ShareDialog::waitLabelHide()
{
    ui->wait->hide();
    ui->toolBar->show();
    ui->stackedWidget->show();
    ui->buttonsWidget->show();
}

void ShareDialog::waitLabelShow()
{
    ui->wait->show();
    ui->toolBar->hide();
    ui->stackedWidget->hide();
    ui->buttonsWidget->hide();
}

void ShareDialog::setRegisterOnly(bool registerOnly)
{
    mRegisterOnly = registerOnly;
}

void ShareDialog::setDialogsOnly(bool dialogsOnly)
{
    mDialogsOnly = dialogsOnly;
}

void ShareDialog::setPrintDialogs(const QString &printDialogs)
{
    mPrintDialogs = printDialogs;
}

void ShareDialog::setSilent(bool silent)
{
    mSilent = silent;
}

void ShareDialog::setChannel(const QString &channel)
{
    mChannel = channel;
    if (channel.contains("{"))
        mPeers << InputPeer::fromJson(channel);
    else
    if (channel.contains("/"))
    {
        QFile file(channel);
        file.open(QFile::ReadOnly);

        QVariantList list;
        QDataStream stream(&file);
        stream >> list;

        file.close();

        for (const QVariant &l: list)
            mPeers << InputPeer::fromMap(l.toMap());
    }
    else
        ui->channelName->setText(channel);
}

bool ShareDialog::getHideAbout() const
{
    return mHideAbout;
}

void ShareDialog::setHideAbout(bool hideAbout)
{
    mHideAbout = hideAbout;
    if (mHideAbout)
        ui->toolBar->removeAction(ui->actionBack);
    else
        ui->toolBar->addAction(ui->actionAbout);
}

void ShareDialog::setPhone(const QString &phone)
{
    mPhone = phone;
    ui->phoneLine->setEditText(mPhone);
    on_nextBtn_clicked();
}

QString ShareDialog::getFile() const
{
    return mFile;
}

void ShareDialog::setFile(const QString &file)
{
    mFile = file;
}

QString ShareDialog::getBody() const
{
    return mBody;
}

void ShareDialog::setBody(const QString &body)
{
    mBody = body;
    ui->captionTextEdit->setPlainText(mBody);
}

void ShareDialog::finish()
{
    ui->stackedWidget->setCurrentIndex(6);
    if (mSilent)
        QCoreApplication::quit();
}

void ShareDialog::getDialogs()
{
    if (mPrintDialogs.isEmpty())
        return;

    mTg->messagesGetDialogs(false, false, false, InputPeer::null, 100, [this](TG_MESSAGES_GET_DIALOGS_CALLBACK){

        QVariantList dialogsList;

        QHash<qint32, User> users;
        for(const User &user: result.users())
            users[user.id()] = user;
        QHash<qint32, Chat> chats;
        for(const Chat &chat: result.chats())
            chats[chat.id()] = chat;
        for(const Dialog &dlg: result.dialogs())
        {
            QVariantMap map;
            map["id"] = QString(dlg.peer().getHash().toHex());

            InputPeer inputPeer;

            switch(static_cast<qint32>(dlg.peer().classType()))
            {
            case Peer::typePeerChannel:
            {
                Chat chat = chats.value(dlg.peer().channelId());
                inputPeer.setClassType(InputPeer::typeInputPeerChannel);
                inputPeer.setChannelId(chat.id());
                inputPeer.setAccessHash(chat.accessHash());

                map["inputPeer"] = inputPeer.toMap();
                map["title"] = chat.title();
            }
                break;
            case Peer::typePeerChat:
            {
                Chat chat = chats.value(dlg.peer().chatId());
                inputPeer.setClassType(InputPeer::typeInputPeerChat);
                inputPeer.setChatId(chat.id());
                inputPeer.setAccessHash(chat.accessHash());

                map["inputPeer"] = inputPeer.toMap();
                map["title"] = chat.title();
            }
                break;
            case Peer::typePeerUser:
            {
                User user = users.value(dlg.peer().userId());
                inputPeer.setClassType(InputPeer::typeInputPeerUser);
                inputPeer.setUserId(user.id());
                inputPeer.setAccessHash(user.accessHash());

                map["inputPeer"] = inputPeer.toMap();
                map["title"] = (user.firstName() + " " + user.lastName()).trimmed();
            }
                break;
            }

            dialogsList << map;
        }

        QFile file(mPrintDialogs);
        file.open(QFile::WriteOnly);

        QDataStream stream(&file);
        stream << dialogsList;

        file.close();
        if (mDialogsOnly)
        {
            QCoreApplication::quit();
            return;
        }
    });
}

QString ShareDialog::homePath()
{
    if(aseman_app_home_path)
        return *aseman_app_home_path;

    aseman_app_home_path = new QString();

    QString oldPath;
#ifdef Q_OS_WIN
    oldPath = QDir::homePath() + "/AppData/Local/" + QCoreApplication::applicationName();
#else
    oldPath = QDir::homePath() + "/.config/" + QCoreApplication::applicationName();
#endif

    QStringList paths = QStandardPaths::standardLocations(QStandardPaths::QStandardPaths::AppDataLocation);
    if(paths.isEmpty())
        paths << oldPath;

    if( oldPath.count() && QFileInfo::exists(oldPath) )
        *aseman_app_home_path = oldPath;
    else
        *aseman_app_home_path = paths.first();

    QDir().mkpath(*aseman_app_home_path);
    return *aseman_app_home_path;
}

qint64 ShareDialog::generateRandomId()
{
    qint64 randomId;
    Utils::randomBytes(&randomId, 8);
    return randomId;
}

ShareDialog::~ShareDialog()
{
    delete ui;
}

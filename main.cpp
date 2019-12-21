/*
    Copyright (C) 2019 Aseman Team
    http://aseman.io

    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This project is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sharedialog.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>

QString getCommandLineValue(const QString &switchStr)
{
    const QStringList args = QCoreApplication::instance()->arguments();
    qint32 idx = args.indexOf(switchStr);
    if (idx < 0)
        return QString();
    if (idx+1 >= args.count())
        return QString("true");

    return args.at(idx+1);
}

void myMessageOutput(QtMsgType, const QMessageLogContext &, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit().trimmed();
    fprintf(stderr, "%s\n", localMsg.constData());
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
        qInstallMessageHandler(myMessageOutput);
#endif
    QApplication app(argc, argv);
    app.setApplicationName("TgSendMessage");
    app.setOrganizationName("Aseman Team");
    app.setApplicationVersion("0.8");
    app.setApplicationDisplayName("Telegram Send Message");
    app.setOrganizationDomain("io.aseman");
    app.setWindowIcon( QIcon(":/icons/icon.png") );

    qputenv("QT_LOGGING_RULES", "tg.*=false");

    if (app.arguments().count() == 1)
    {
        qDebug() << QString("Usage: " + QFileInfo(app.applicationFilePath()).fileName() + " --body \"message to send\" --file \"file to send\"").toStdString().c_str();
        return 0;
    }
    bool phones = getCommandLineValue("--phones").length();
    if (phones)
    {
        qDebug() << QDir(ShareDialog::homePath()).entryList(QDir::Dirs | QDir::NoDotAndDotDot).join("\n").toStdString().c_str();
        return 0;
    }

    QString body = getCommandLineValue("--body");
    QString file = getCommandLineValue("--file");
    QString phone = getCommandLineValue("--phone");
    QString channel = getCommandLineValue("--channel");
    bool hideAbout = getCommandLineValue("--hide-about").length();
    bool silent = getCommandLineValue("--silent").length();
    QString printDialogs = getCommandLineValue("--printDialogs");
    bool dialogsOnly = getCommandLineValue("--dialogsOnly").length();
    bool registerOnly = getCommandLineValue("--registerOnly").length();

    ShareDialog w;
    w.setBody(body);
    w.setFile(file);
    w.setHideAbout(hideAbout);
    w.setPrintDialogs(printDialogs);
    w.setDialogsOnly(dialogsOnly);
    w.setRegisterOnly(registerOnly);
    if (phone.length()) w.setPhone(phone);
    if (channel.length()) w.setChannel(channel);
    w.setSilent(silent && phone.length());

    if (!silent || phone.isEmpty()) w.show();

    return app.exec();
}

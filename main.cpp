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
#include <QFileInfo>

int main(int argc, char *argv[])
{
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

    qint32 bodyIdx = app.arguments().indexOf("--body");
    QString body = (bodyIdx>=0? app.arguments().at(bodyIdx+1) : "");

    qint32 fileIdx = app.arguments().indexOf("--file");
    QString file = (fileIdx>=0? app.arguments().at(fileIdx+1) : "");

    bool hideAbout = (app.arguments().indexOf("--hide-about") != -1);

    ShareDialog w;
    w.setBody(body);
    w.setFile(file);
    w.setHideAbout(hideAbout);
    w.show();

    return app.exec();
}

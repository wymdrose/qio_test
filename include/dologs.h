#ifndef DOLOGS_H
#define DOLOGS_H

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

namespace Dologs
{

inline void outlog(const QString& msg)
{
    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << msg;
}

inline void outputMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);

    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("Debug: %1").arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1").arg(msg);
        break;
    case QtCriticalMsg:
        txt = QString("Critical: %1").arg(msg);
        break;
    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(msg);
        break;
    case QtInfoMsg:
        txt = QString("Info: %1").arg(msg);
        break;
    }

    QFile outFile(QCoreApplication::applicationDirPath() + "/log.txt");
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream ts(&outFile);
        ts << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ") << txt << Qt::endl;
    }
}

}  // namespace Dologs

#endif // DOLOGS_H

#ifndef DOLOGS_H
#define DOLOGS_H

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>
#include <QMutex>

namespace Dologs
{

inline void outlog(const QString& msg)
{
    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << msg;
}

inline QString defaultLogFilePath()
{
    const QString baseDir = QCoreApplication::applicationDirPath() + "/logs";
    QDir().mkpath(baseDir);
    const QString date = QDate::currentDate().toString("yyyyMMdd");
    return baseDir + QString("/qio_%1.log").arg(date);
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

    static QMutex sMutex;
    QMutexLocker locker(&sMutex);

    QFile outFile(defaultLogFilePath());
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&outFile);
        ts << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ") << txt << Qt::endl;
        ts.flush();
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

inline void installQtMessageHandler()
{
    qInstallMessageHandler(outputMessage);
}

}  // namespace Dologs

#endif // DOLOGS_H

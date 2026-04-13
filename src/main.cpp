#include "qiotest.h"
#include <QApplication>
#include <QDebug>
#include "dologs.h"

// Global variables
std::shared_ptr<DatabaseCover::MySqLite> gpDoSqlite;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // MSVC builds use /utf-8; Qt 5 source encoding follows the build.
    QCoreApplication::setOrganizationName("Dongguan Jingwei");
    QCoreApplication::setApplicationName("QIoTest");
    QCoreApplication::setApplicationVersion("4.0.x");

    Dologs::installQtMessageHandler();

    QIoTest* w = new QIoTest();
    w->showMaximized();
    w->show();

    // Initialize SQLite database
    gpDoSqlite = std::make_shared<DatabaseCover::MySqLite>("dosqlite");
    qDebug() << "begin......";
    gpDoSqlite->open();

    return a.exec();
}

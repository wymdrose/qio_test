#include "qiotest.h"
#include <QApplication>
#include <QDebug>

// Global variables
std::shared_ptr<DatabaseCover::MySqLite> gpDoSqlite;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set UTF-8 encoding (Qt6 uses UTF-8 by default)
    QCoreApplication::setOrganizationName("Dongguan Jingwei");
    QCoreApplication::setApplicationName("QIoTest");
    QCoreApplication::setApplicationVersion("3.0.12");

    QIoTest* w = new QIoTest();
    w->showMaximized();
    w->show();

    // Initialize SQLite database
    gpDoSqlite = std::make_shared<DatabaseCover::MySqLite>("dosqlite");
    qDebug() << "begin......";
    gpDoSqlite->open();

    return a.exec();
}

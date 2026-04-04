#ifndef GLOBAL_H
#define GLOBAL_H

#include <QMainWindow>
#include "ui_qiotest.h"
#include <memory>
#include <QPushButton>
#include <QGroupBox>
#include <QProcess>
#include <QSettings>
#include <QSqlTableModel>

#include "communicatelib.h"
#include "xlsxfile.h"
#include "mysignal.h"
#include "mysqlite.h"
#include "dologs.h"

// Global declarations
extern Ui::QIoTestClass* gpUi;
extern QString gExePath;
extern std::shared_ptr<Drose::MySignalUi> gpSignal;
extern std::vector<QStringList> ipVector;
extern std::vector<CommunicateClass::TcpClient*> gpTcpClientVector;
extern std::shared_ptr<CommunicateClass::ComPortOne> gpComClient;
extern std::shared_ptr<DatabaseCover::MySqLite> gpDoSqlite;

#define GLOBAL \
Ui::QIoTestClass* gpUi; \
QString gExePath; \
std::shared_ptr<Drose::MySignalUi> gpSignal; \
std::vector<QStringList> ipVector; \
std::vector<CommunicateClass::TcpClient*> gpTcpClientVector; \
std::shared_ptr<CommunicateClass::ComPortOne> gpComClient;

#endif // GLOBAL_H

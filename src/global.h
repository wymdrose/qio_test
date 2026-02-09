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

#ifdef HAVE_SERIALBUS
#include <QModbusDataUnit>
#include <QModbusTcpClient>
#endif

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
#ifdef HAVE_SERIALBUS
extern std::shared_ptr<QModbusClient> gpModbusDevice;
#endif

#ifdef HAVE_SERIALBUS
#define GLOBAL \
Ui::QIoTestClass* gpUi; \
QString gExePath; \
std::shared_ptr<Drose::MySignalUi> gpSignal; \
std::vector<QStringList> ipVector; \
std::vector<CommunicateClass::TcpClient*> gpTcpClientVector; \
std::shared_ptr<CommunicateClass::ComPortOne> gpComClient; \
std::shared_ptr<QModbusClient> gpModbusDevice;
#else
#define GLOBAL \
Ui::QIoTestClass* gpUi; \
QString gExePath; \
std::shared_ptr<Drose::MySignalUi> gpSignal; \
std::vector<QStringList> ipVector; \
std::vector<CommunicateClass::TcpClient*> gpTcpClientVector; \
std::shared_ptr<CommunicateClass::ComPortOne> gpComClient;
#endif

#endif // GLOBAL_H

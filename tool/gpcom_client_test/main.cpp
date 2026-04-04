#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <memory>
#include "communicatelib.h"

static QTextStream& qout()
{
    static QTextStream s(stdout);
    return s;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("gpcom_client_test");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Sends AA2800 (read point status) through CommunicateClass::ComPortOne (same path as QIoTest::slotFind).\n"
        "\n"
        "Typical setup (Windows): install com0com, create a pair e.g. COM5 <-> COM6.\n"
        "  Terminal 1: VspServer.exe -p COM5\n"
        "  Terminal 2: gpcom_client_test.exe -p 6");
    parser.addHelpOption();
    QCommandLineOption portOpt(QStringList() << "p" << "com-port",
                               "COM port number (6 means COM6), matching QIoTest combo index.",
                               "n", "6");
    parser.addOption(portOpt);
    parser.process(app);

    bool ok = false;
    unsigned int portNo = parser.value(portOpt).toUInt(&ok);
    if (!ok || portNo == 0 || portNo > 255) {
        qout() << "Invalid COM port number.\n";
        return 2;
    }

    auto client = std::make_shared<CommunicateClass::ComPortOne>(portNo);
    if (!client->init()) {
        qout() << QString("Failed to open COM%1. Use another port, close other apps, or start VspServer on the paired COM first.\n")
                      .arg(portNo);
        return 1;
    }

    QByteArray recv;
    if (!client->communicate(QByteArray::fromHex("AA2800"), recv)) {
        qout() << "communicate() returned false.\n";
        return 1;
    }

    if (recv.isEmpty() || static_cast<uint8_t>(recv[0]) != 0xDE) {
        qout() << QString("Unexpected response (%1 bytes): %2\n")
                      .arg(recv.size())
                      .arg(QString(recv.toHex(' ').toUpper()));
        return 1;
    }

    qout() << QString("OK  COM%1  RX %2 bytes: %3\n")
                  .arg(portNo)
                  .arg(recv.size())
                  .arg(QString(recv.toHex(' ').toUpper()));
    return 0;
}

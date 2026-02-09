#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QSerialPortInfo>
#include <QTextStream>
#include <QDebug>
#include "vspserver.h"

static QTextStream& sout()
{
    static QTextStream s(stdout);
    return s;
}

static void printAvailablePorts()
{
    sout() << "\nAvailable serial ports:\n";
    const auto ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty()) {
        sout() << "  (none found)\n";
    } else {
        for (const auto& port : ports) {
            sout() << QString("  %1  -  %2  [%3:%4]\n")
                       .arg(port.portName(), -10)
                       .arg(port.description())
                       .arg(port.vendorIdentifier(), 4, 16, QChar('0'))
                       .arg(port.productIdentifier(), 4, 16, QChar('0'));
        }
    }
    sout() << "\n";
    sout().flush();
}

// ---- Self-test: exercise all protocol handlers without a real serial port ----
static int runSelfTest()
{
    sout() << "\n[ Self-Test ] Running protocol handler tests...\n\n";
    sout().flush();

    VspServer server("NONE");   // port name unused for self-test
    server.setVersion(560);     // V5.60
    server.setTestStatus(0x11);
    server.setPassCount(100);
    server.setFailCount(5);
    server.setUse2BytePoints(true);

    int pass = 0, fail = 0;

    auto check = [&](const QString& name, const QByteArray& cmd,
                     bool (*validate)(const QByteArray&)) {
        QByteArray resp = server.handleCommand(cmd);
        bool ok = validate(resp);
        if (ok) {
            sout() << QString("  PASS  %1  TX:%2  RX:%3\n")
                      .arg(name, -35)
                      .arg(QString(cmd.toHex(' ').toUpper()), -30)
                      .arg(QString(resp.toHex(' ').toUpper()));
            pass++;
        } else {
            sout() << QString("  FAIL  %1  TX:%2  RX:%3\n")
                      .arg(name, -35)
                      .arg(QString(cmd.toHex(' ').toUpper()), -30)
                      .arg(QString(resp.toHex(' ').toUpper()));
            fail++;
        }
        sout().flush();
    };

    // 1. AA 01 - Read version -> DD 02 30 (560 = 0x0230)
    check("AA 01  Read Version",
          QByteArray::fromHex("AA01"),
          [](const QByteArray& r) {
              return r.size() == 3
                  && static_cast<uint8_t>(r[0]) == 0xDD
                  && static_cast<uint8_t>(r[1]) == 0x02
                  && static_cast<uint8_t>(r[2]) == 0x30;
          });

    // 2. AA 02 - Read date/time -> DD + 7 bytes
    check("AA 02  Read DateTime",
          QByteArray::fromHex("AA02"),
          [](const QByteArray& r) {
              return r.size() == 8
                  && static_cast<uint8_t>(r[0]) == 0xDD;
          });

    // 3. AA 03 - Read file count -> DD 00 03 (3 default files)
    check("AA 03  Read FileCount",
          QByteArray::fromHex("AA03"),
          [](const QByteArray& r) {
              return r.size() == 3
                  && static_cast<uint8_t>(r[0]) == 0xDD
                  && static_cast<uint8_t>(r[1]) == 0x00
                  && static_cast<uint8_t>(r[2]) == 0x03;
          });

    // 4. AA 04 00 01 - Read file name fixed (file #1) -> DD + 40 bytes "test-pin"
    check("AA 04  Read FileName Fixed #1",
          QByteArray::fromHex("AA040001"),
          [](const QByteArray& r) {
              return r.size() == 41
                  && static_cast<uint8_t>(r[0]) == 0xDD
                  && r.mid(1, 8) == QByteArray("test-pin");
          });

    // 5. AA 05 00 02 - Read file name variable (file #2) -> DD 08 "mul-test"
    check("AA 05  Read FileName Var #2",
          QByteArray::fromHex("AA050002"),
          [](const QByteArray& r) {
              return r.size() >= 2
                  && static_cast<uint8_t>(r[0]) == 0xDD
                  && static_cast<uint8_t>(r[1]) == 8
                  && r.mid(2) == QByteArray("mul-test");
          });

    // 6. AA 06 - Read test count -> DD + 4 bytes pass + 4 bytes fail
    check("AA 06  Read TestCount",
          QByteArray::fromHex("AA06"),
          [](const QByteArray& r) {
              if (r.size() != 9 || static_cast<uint8_t>(r[0]) != 0xDD) return false;
              uint32_t p = (static_cast<uint8_t>(r[1]) << 24) | (static_cast<uint8_t>(r[2]) << 16)
                         | (static_cast<uint8_t>(r[3]) << 8) | static_cast<uint8_t>(r[4]);
              uint32_t f = (static_cast<uint8_t>(r[5]) << 24) | (static_cast<uint8_t>(r[6]) << 16)
                         | (static_cast<uint8_t>(r[7]) << 8) | static_cast<uint8_t>(r[8]);
              return p == 100 && f == 5;
          });

    // 7. AA 07 - Read test status -> DD 11
    check("AA 07  Read TestStatus",
          QByteArray::fromHex("AA07"),
          [](const QByteArray& r) {
              return r.size() == 2
                  && static_cast<uint8_t>(r[0]) == 0xDD
                  && static_cast<uint8_t>(r[1]) == 0x11;
          });

    // 8. AA 07 A1 - Set auto-report option -> DD A1
    check("AA 07 A1  TestStatus AutoReport",
          QByteArray::fromHex("AA07A1"),
          [](const QByteArray& r) {
              return r.size() == 2
                  && static_cast<uint8_t>(r[0]) == 0xDD
                  && static_cast<uint8_t>(r[1]) == 0xA1;
          });

    // 9. AA 08 01 - Simulate button -> DD 01
    check("AA 08  Simulate Button",
          QByteArray::fromHex("AA0801"),
          [](const QByteArray& r) {
              return r.size() == 2
                  && static_cast<uint8_t>(r[0]) == 0xDD
                  && static_cast<uint8_t>(r[1]) == 0x01;
          });

    // 10. AA 09 - Unlock -> DD 01
    check("AA 09  Unlock",
          QByteArray::fromHex("AA09"),
          [](const QByteArray& r) {
              return r.size() == 2
                  && static_cast<uint8_t>(r[0]) == 0xDD
                  && static_cast<uint8_t>(r[1]) == 0x01;
          });

    // 11. AA 0A - Lock -> DD 00
    check("AA 0A  Lock",
          QByteArray::fromHex("AA0A"),
          [](const QByteArray& r) {
              return r.size() == 2
                  && static_cast<uint8_t>(r[0]) == 0xDD
                  && static_cast<uint8_t>(r[1]) == 0x00;
          });

    // 12. AA 10 + "test-pin" - Open file -> DD 00
    {
        QByteArray cmd = QByteArray::fromHex("AA10") + QByteArray("test-pin");
        check("AA 10  OpenFile 'test-pin'",
              cmd,
              [](const QByteArray& r) {
                  return r.size() == 2
                      && static_cast<uint8_t>(r[0]) == 0xDD
                      && static_cast<uint8_t>(r[1]) == 0x00;
              });
    }

    // 13. AA 10 + "nonexistent" - Open file -> DD FF
    {
        QByteArray cmd = QByteArray::fromHex("AA10") + QByteArray("nonexistent");
        check("AA 10  OpenFile not found",
              cmd,
              [](const QByteArray& r) {
                  return r.size() == 2
                      && static_cast<uint8_t>(r[0]) == 0xDD
                      && static_cast<uint8_t>(r[1]) == 0xFF;
              });
    }

    // 14. AA 20 00 - Read sample data -> DE + data (2-byte mode)
    check("AA 20  Read SampleData",
          QByteArray::fromHex("AA2000"),
          [](const QByteArray& r) {
              return r.size() > 1
                  && static_cast<uint8_t>(r[0]) == 0xDE;
          });

    // 15. AA 22 00 - Read NG detail (no defects) -> DE (empty)
    check("AA 22  Read NgDetail (empty)",
          QByteArray::fromHex("AA2200"),
          [](const QByteArray& r) {
              return r.size() == 1
                  && static_cast<uint8_t>(r[0]) == 0xDE;
          });

    // 16. AA 23 00 - Scan sample -> DD 00
    check("AA 23  ScanSample",
          QByteArray::fromHex("AA2300"),
          [](const QByteArray& r) {
              return r.size() == 2
                  && static_cast<uint8_t>(r[0]) == 0xDD
                  && static_cast<uint8_t>(r[1]) == 0x00;
          });

    // 17. AA 25 - Read file options -> DD + 26 bytes
    check("AA 25  Read FileOptions",
          QByteArray::fromHex("AA25"),
          [](const QByteArray& r) {
              return r.size() == 27
                  && static_cast<uint8_t>(r[0]) == 0xDD;
          });

    // 18. AA 26 - Read file options V5.47 -> DD + 28 bytes
    check("AA 26  Read FileOptions V5.47",
          QByteArray::fromHex("AA26"),
          [](const QByteArray& r) {
              return r.size() == 29
                  && static_cast<uint8_t>(r[0]) == 0xDD;
          });

    // 19. AA 27 00 - Continuity test -> DE + data, pass count incremented
    check("AA 27  ContinuityTest",
          QByteArray::fromHex("AA2700"),
          [](const QByteArray& r) {
              return r.size() > 1
                  && static_cast<uint8_t>(r[0]) == 0xDE;
          });

    // 20. AA 28 00 - Read point status -> DE + pin data
    check("AA 28  Read PointStatus",
          QByteArray::fromHex("AA2800"),
          [](const QByteArray& r) {
              return r.size() >= 1
                  && static_cast<uint8_t>(r[0]) == 0xDE;
          });

    // 21. AA 06 + write test count (need unlock first)
    server.handleCommand(QByteArray::fromHex("AA09")); // unlock
    check("AA 06  Write TestCount",
          QByteArray::fromHex("AA0600000001000000020000000300000004"),
          // The handler reads 10 bytes: AA 06 + 8 data bytes
          // But handleCommand gets the full pre-parsed command,
          // so we pass exactly AA 06 + 4 pass + 4 fail = 10 bytes
          [](const QByteArray& r) {
              // Should return DD + 4+4 bytes with the new counts
              return r.size() == 9
                  && static_cast<uint8_t>(r[0]) == 0xDD;
          });

    // Fix: the write test count command was too long. Let me also test with correct 10 bytes
    check("AA 06  Write TestCount (10B)",
          QByteArray::fromHex("AA06000000C8000000FF"),
          [](const QByteArray& r) {
              if (r.size() != 9 || static_cast<uint8_t>(r[0]) != 0xDD) return false;
              uint32_t p = (static_cast<uint8_t>(r[1]) << 24) | (static_cast<uint8_t>(r[2]) << 16)
                         | (static_cast<uint8_t>(r[3]) << 8) | static_cast<uint8_t>(r[4]);
              uint32_t f = (static_cast<uint8_t>(r[5]) << 24) | (static_cast<uint8_t>(r[6]) << 16)
                         | (static_cast<uint8_t>(r[7]) << 8) | static_cast<uint8_t>(r[8]);
              return p == 200 && f == 255;
          });

    // 22. AA 24 - Reset sample
    check("AA 24  ResetSample",
          QByteArray::fromHex("AA24"),
          [](const QByteArray& r) {
              return r.size() == 1
                  && static_cast<uint8_t>(r[0]) == 0xDD;
          });

    // 23. AA 11 - Save file (new name)
    {
        QByteArray cmd = QByteArray::fromHex("AA11") + QByteArray("new-file");
        check("AA 11  SaveFile 'new-file'",
              cmd,
              [](const QByteArray& r) {
                  return r.size() == 2
                      && static_cast<uint8_t>(r[0]) == 0xDD
                      && static_cast<uint8_t>(r[1]) == 0x00;
              });
    }

    // 24. AA 11 - Save file (duplicate) -> DD FF
    {
        QByteArray cmd = QByteArray::fromHex("AA11") + QByteArray("new-file");
        check("AA 11  SaveFile duplicate",
              cmd,
              [](const QByteArray& r) {
                  return r.size() == 2
                      && static_cast<uint8_t>(r[0]) == 0xDD
                      && static_cast<uint8_t>(r[1]) == 0xFF;
              });
    }

    sout() << "\n----------------------------------------------\n";
    sout() << QString("  Results: %1 passed, %2 failed, %3 total\n")
                  .arg(pass).arg(fail).arg(pass + fail);
    sout() << "----------------------------------------------\n\n";
    sout().flush();

    return fail == 0 ? 0 : 1;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("VspServer");
    QCoreApplication::setApplicationVersion("1.0.0");

    QTextStream out(stdout);

    out << "==============================================\n";
    out << "  Virtual Serial Port Server v1.0.0\n";
    out << "  Protocol: Wire Continuity Tester\n";
    out << "  Baud: 115200 / Even Parity / 1 Stop Bit\n";
    out << "==============================================\n";
    out.flush();

    // Command line options
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Virtual Serial Port Server - simulates a wire continuity tester instrument.\n"
        "Listens on the specified COM port and responds to protocol commands.\n\n"
        "Tip: Use a virtual serial port pair (e.g. com0com) to connect this server\n"
        "     to the QIoTest application for development/testing.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(
        QStringList() << "p" << "port",
        "Serial port name (e.g. COM5, /dev/ttyS0).",
        "port", "COM5");
    parser.addOption(portOption);

    QCommandLineOption versionNumOption(
        QStringList() << "V" << "device-version",
        "Simulated device version number (decimal, e.g. 560 for V5.60).",
        "version", "560");
    parser.addOption(versionNumOption);

    QCommandLineOption statusOption(
        QStringList() << "s" << "test-status",
        "Initial test status (hex, e.g. 11=inserted+OK, 12=inserted+NG, 00=no wire).",
        "status", "11");
    parser.addOption(statusOption);

    QCommandLineOption passOption(
        "pass",
        "Initial pass count.",
        "count", "100");
    parser.addOption(passOption);

    QCommandLineOption failOption(
        "fail",
        "Initial fail count.",
        "count", "5");
    parser.addOption(failOption);

    QCommandLineOption mode1ByteOption(
        "1byte",
        "Use 1-byte point mode (DD). Default is 2-byte mode (DE).");
    parser.addOption(mode1ByteOption);

    QCommandLineOption tcpOption(
        "tcp",
        "Run in TCP server mode on the specified port (no serial port needed).",
        "tcpport", "0");
    parser.addOption(tcpOption);

    QCommandLineOption listOption(
        QStringList() << "l" << "list",
        "List available serial ports and exit.");
    parser.addOption(listOption);

    QCommandLineOption selfTestOption(
        QStringList() << "t" << "self-test",
        "Run internal self-test of all protocol handlers and exit.");
    parser.addOption(selfTestOption);

    QCommandLineOption fileOption(
        QStringList() << "f" << "file",
        "Add a virtual file name (can be specified multiple times).",
        "name");
    parser.addOption(fileOption);

    QCommandLineOption sampleOption(
        "sample",
        "Add sample data point pair as 'output:input1,input2,...' in hex.\n"
        "Example: --sample 0000:0040  (A001->B001)\n"
        "Can be specified multiple times.",
        "data");
    parser.addOption(sampleOption);

    QCommandLineOption ngOption(
        "ng",
        "Add an NG defect as 'code:pinA:pinB' in hex.\n"
        "Codes: 01=open 02=intermittent 03=short 04=int-short 05=misalign 06=cross\n"
        "Example: --ng 05:0001:0007  (misalign A002>A008)\n"
        "Can be specified multiple times.",
        "defect");
    parser.addOption(ngOption);

    parser.process(app);

    if (parser.isSet(listOption)) {
        printAvailablePorts();
        return 0;
    }

    if (parser.isSet(selfTestOption)) {
        return runSelfTest();
    }

    QString portName = parser.value(portOption);
    uint16_t deviceVersion = static_cast<uint16_t>(parser.value(versionNumOption).toUInt());
    bool ok;
    uint8_t testStatus = static_cast<uint8_t>(parser.value(statusOption).toUInt(&ok, 16));
    uint32_t passCount = parser.value(passOption).toUInt();
    uint32_t failCount = parser.value(failOption).toUInt();
    bool use1Byte = parser.isSet(mode1ByteOption);

    // Create server
    VspServer server(portName);
    server.setVersion(deviceVersion);
    server.setTestStatus(testStatus);
    server.setPassCount(passCount);
    server.setFailCount(failCount);
    server.setUse2BytePoints(!use1Byte);

    // Add custom files
    QStringList customFiles = parser.values(fileOption);
    for (const auto& f : customFiles) {
        server.addFile(f);
    }

    // Add sample data
    QStringList sampleEntries = parser.values(sampleOption);
    for (const auto& entry : sampleEntries) {
        // Format: output:input1,input2,...
        QStringList parts = entry.split(':');
        if (parts.size() >= 2) {
            uint16_t outPin = static_cast<uint16_t>(parts[0].toUInt(&ok, 16));
            QVector<uint16_t> inPins;
            QStringList inParts = parts[1].split(',');
            for (const auto& ip : inParts) {
                inPins.append(static_cast<uint16_t>(ip.toUInt(&ok, 16)));
            }
            server.addSamplePoint(outPin, inPins);
        }
    }

    // Add NG defects
    QStringList ngEntries = parser.values(ngOption);
    QVector<VspServer::NgDefect> defects;
    for (const auto& entry : ngEntries) {
        // Format: code:pinA:pinB
        QStringList parts = entry.split(':');
        if (parts.size() >= 3) {
            VspServer::NgDefect defect;
            defect.code = static_cast<uint8_t>(parts[0].toUInt(&ok, 16));
            defect.pinA = static_cast<uint16_t>(parts[1].toUInt(&ok, 16));
            defect.pinB = static_cast<uint16_t>(parts[2].toUInt(&ok, 16));
            defects.append(defect);
        }
    }
    if (!defects.isEmpty()) {
        server.setTestNgDefects(defects);
    }

    // Start in TCP or serial mode
    bool tcpMode = parser.isSet(tcpOption) && parser.value(tcpOption) != "0";

    if (tcpMode) {
        quint16 tcpPort = static_cast<quint16>(parser.value(tcpOption).toUInt());
        out << QString("Starting TCP server on port %1...\n").arg(tcpPort);
        out.flush();

        if (!server.startTcp(tcpPort)) {
            out << "Failed to start TCP server.\n";
            out.flush();
            return 1;
        }

        out << QString("TCP server running on port %1. Press Ctrl+C to stop.\n\n").arg(tcpPort);
        out.flush();
    } else {
        // Serial port mode
        printAvailablePorts();

        out << QString("Starting server on %1...\n").arg(portName);
        out.flush();

        if (!server.start()) {
            out << "Failed to start server. Check that the port exists and is not in use.\n";
            out << "Tip: Use --tcp <port> to run in TCP mode without a serial port.\n";
            out.flush();
            return 1;
        }

        out << "Server running. Press Ctrl+C to stop.\n\n";
        out.flush();
    }

    return app.exec();
}

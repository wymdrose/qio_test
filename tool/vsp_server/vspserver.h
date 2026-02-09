#ifndef VSPSERVER_H
#define VSPSERVER_H

#include <QObject>
#include <QSerialPort>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QDateTime>
#include <QMap>
#include <QVector>
#include <QTimer>
#include <cstdint>

/**
 * VspServer - Virtual Serial Port Server
 *
 * Simulates the wire continuity tester instrument protocol as documented in:
 *   "串口接线和指令说明20250818.pdf"
 *
 * Protocol summary:
 *   - Baud rate: 115200, Even parity, 1 stop bit
 *   - Commands:  AA <cmd> [params...]
 *   - Responses: DD [data...]  (or DE for 2-byte point mode)
 *
 * Supported commands:
 *   AA 01       Read system version
 *   AA 02       Read system date/time
 *   AA 03       Read file count
 *   AA 04 xxxx  Read file name (fixed 40 bytes)
 *   AA 05 xxxx  Read file name (variable length)
 *   AA 06       Read test count
 *   AA 06 +8B   Write test count
 *   AA 07 [opt] Read test status
 *   AA 08 xx    Simulate button press
 *   AA 09 [pw]  Unlock
 *   AA 0A       Lock
 *   AA 10 [fn]  Open file
 *   AA 11 [fn]  Save file
 *   AA 20 xx    Read sample data
 *   AA 21 xx    Read test data
 *   AA 22 xx    Read NG detail
 *   AA 23 xx    Scan sample data
 *   AA 24       Reset sample data
 *   AA 25       Read file options (26 bytes)
 *   AA 25 +data Write file options
 *   AA 26       Read file options V5.47+ (28 bytes)
 *   AA 26 +data Write file options V5.47+
 *   AA 27 xx    Continuity test & read data
 *   AA 28 xx    Read point status
 */
class VspServer : public QObject
{
    Q_OBJECT

public:
    explicit VspServer(const QString& portName, QObject* parent = nullptr);
    ~VspServer();

    bool start();           // Start on serial port
    bool startTcp(quint16 port);  // Start on TCP port
    void stop();

    // Configuration
    void setVersion(uint16_t version) { m_version = version; }
    void setTestStatus(uint8_t status) { m_testStatus = status; }
    void setPassCount(uint32_t count) { m_passCount = count; }
    void setFailCount(uint32_t count) { m_failCount = count; }
    void setUse2BytePoints(bool use2Byte) { m_use2BytePoints = use2Byte; }

    // Add virtual files
    void addFile(const QString& name);

    // Add sample data point pair (output pin -> list of input pins)
    void addSamplePoint(uint16_t outputPin, const QVector<uint16_t>& inputPins);

    // Set test result to simulate: true=OK, false=NG with specified defects
    struct NgDefect {
        uint8_t code;       // 01=open, 02=intermittent, 03=short, 04=intermittent short, 05=misalign, 06=cross
        uint16_t pinA;
        uint16_t pinB;
    };
    void setTestNgDefects(const QVector<NgDefect>& defects);

    // Process a raw command and return the response (also used for self-test)
    QByteArray handleCommand(const QByteArray& cmd);

signals:
    void logMessage(const QString& msg);

private slots:
    void onReadyRead();
    void onTcpNewConnection();
    void onTcpClientReadyRead();
    void onTcpClientDisconnected();

private:

    QByteArray cmdReadVersion();
    QByteArray cmdReadDateTime();
    QByteArray cmdReadFileCount();
    QByteArray cmdReadFileNameFixed(uint16_t fileNo);
    QByteArray cmdReadFileNameVariable(uint16_t fileNo);
    QByteArray cmdReadTestCount();
    QByteArray cmdWriteTestCount(uint32_t pass, uint32_t fail);
    QByteArray cmdReadTestStatus(const QByteArray& options);
    QByteArray cmdSimulateButton(uint8_t keyCode);
    QByteArray cmdUnlock(const QByteArray& password);
    QByteArray cmdLock();
    QByteArray cmdOpenFile(const QByteArray& filename);
    QByteArray cmdSaveFile(const QByteArray& filename);
    QByteArray cmdReadSampleData(uint8_t position);
    QByteArray cmdReadTestData(uint8_t position);
    QByteArray cmdReadNgDetail(uint8_t position);
    QByteArray cmdScanSample(uint8_t seekCount);
    QByteArray cmdResetSample();
    QByteArray cmdReadFileOptions();
    QByteArray cmdWriteFileOptions(const QByteArray& data);
    QByteArray cmdReadFileOptionsV547();
    QByteArray cmdWriteFileOptionsV547(const QByteArray& data);
    QByteArray cmdContinuityTest(uint8_t position);
    QByteArray cmdReadPointStatus(uint8_t position);

    // Helpers
    QByteArray buildSampleDataPacket() const;
    void processBuffer(QByteArray& buffer, QIODevice* outputDevice);
    void log(const QString& msg);

    QSerialPort* m_serial = nullptr;
    QString m_portName;
    QByteArray m_rxBuffer;

    // TCP mode
    QTcpServer* m_tcpServer = nullptr;
    QTcpSocket* m_tcpClient = nullptr;
    QByteArray m_tcpRxBuffer;

    // Simulated state
    uint16_t m_version = 0x0560;    // V5.60
    bool m_unlocked = false;
    uint32_t m_passCount = 100;
    uint32_t m_failCount = 5;
    uint8_t m_testStatus = 0x11;    // wire inserted, OK
    uint8_t m_autoReportOption = 0xA0;
    int m_currentFileIndex = 0;

    QStringList m_files;

    // Sample data: list of (output_pin, [input_pins...])
    struct SampleStep {
        uint16_t outputPin;
        QVector<uint16_t> inputPins;
    };
    QVector<SampleStep> m_sampleData;
    int m_sampleDataPos = 0;

    // NG defects
    QVector<NgDefect> m_ngDefects;
    int m_ngDataPos = 0;

    // Point status (low-level pins)
    QVector<uint16_t> m_lowPins;

    bool m_use2BytePoints = true;  // DE mode by default

    // File options (26 or 28 bytes)
    QByteArray m_fileOptions;
    QByteArray m_fileOptionsV547;
};

#endif // VSPSERVER_H

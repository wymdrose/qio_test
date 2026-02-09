#include "vspserver.h"
#include <QSerialPortInfo>
#include <QCoreApplication>
#include <QHostAddress>
#include <QDebug>

VspServer::VspServer(const QString& portName, QObject* parent)
    : QObject(parent)
    , m_portName(portName)
{
    m_serial = new QSerialPort(this);

    // Initialize default file options (26 bytes)
    // ContinuousTestTime(4) + DelayCountTime(2) + WaitTimeout(2) + TriggerDelay(2)
    // + TestTimeout(2) + TestType(1) + TestPointCount(2) + NGLock(1) + CopperShort(1)
    // + NGRetestPos(1) + IntPromptCount(2) + ScanDelay(2) + PointMode(1)
    // + PassDotCount(1) + PassDotDuration(1) + PassDotInterval(1)
    m_fileOptions = QByteArray(26, 0x00);
    m_fileOptions[5]  = 0x02;  // DelayCountTime = 0.2s
    m_fileOptions[9]  = 0x01;  // TriggerDelay = 0.1s
    m_fileOptions[12] = 0x01;  // TestType = 1 (continuity)
    m_fileOptions[13] = 0x01;  // TestPointCount high = 256
    m_fileOptions[14] = 0x00;
    m_fileOptions[22] = static_cast<char>(0xFF);  // PointMode = default
    m_fileOptions[24] = 0x05;  // PassDotDuration
    m_fileOptions[25] = 0x0A;  // PassDotInterval

    // Initialize V5.47+ file options (28 bytes) - same base + 2 extra bytes
    m_fileOptionsV547 = QByteArray(28, 0x00);
    for (int i = 0; i < 23; i++)
        m_fileOptionsV547[i] = m_fileOptions[i];
    m_fileOptionsV547[23] = 0x00;  // PassDotCount
    m_fileOptionsV547[24] = 0x00;  // PassDotDuration (2 bytes)
    m_fileOptionsV547[25] = 0x05;
    m_fileOptionsV547[26] = 0x00;  // PassDotInterval (2 bytes)
    m_fileOptionsV547[27] = 0x0A;

    // Default files
    m_files << "test-pin" << "mul-test" << "demo-001";

    // Default sample data (2 pairs: A001<->B001)
    SampleStep s1; s1.outputPin = 0x0000; s1.inputPins = {0x0040};
    SampleStep s2; s2.outputPin = 0x0040; s2.inputPins = {0x0000};
    m_sampleData.append(s1);  // A001 -> B001
    m_sampleData.append(s2);  // B001 -> A001

    // Default low pins for point status
    m_lowPins << 0x0001 << 0x0002;  // A002, A003
}

VspServer::~VspServer()
{
    stop();
}

bool VspServer::start()
{
    m_serial->setPortName(m_portName);
    m_serial->setBaudRate(115200);
    m_serial->setParity(QSerialPort::EvenParity);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        log(QString("ERROR: Failed to open %1: %2").arg(m_portName, m_serial->errorString()));
        return false;
    }

    connect(m_serial, &QSerialPort::readyRead, this, &VspServer::onReadyRead);

    log(QString("Virtual Serial Port Server started on %1").arg(m_portName));
    log(QString("  Baud: 115200, Parity: Even, StopBits: 1"));
    log(QString("  Version: V%1.%2")
        .arg(m_version >> 8)
        .arg(m_version & 0xFF, 2, 10, QChar('0')));
    log(QString("  Files: %1").arg(m_files.join(", ")));
    log(QString("  Sample points: %1 steps").arg(m_sampleData.size()));
    log(QString("  Mode: %1").arg(m_use2BytePoints ? "DE (2-byte points)" : "DD (1-byte points)"));
    log(QString("Waiting for commands..."));

    return true;
}

bool VspServer::startTcp(quint16 port)
{
    m_tcpServer = new QTcpServer(this);

    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        log(QString("ERROR: Failed to listen on TCP port %1: %2")
            .arg(port).arg(m_tcpServer->errorString()));
        return false;
    }

    connect(m_tcpServer, &QTcpServer::newConnection, this, &VspServer::onTcpNewConnection);

    log(QString("Virtual Serial Port Server started on TCP port %1").arg(port));
    log(QString("  Version: V%1.%2")
        .arg(m_version >> 8)
        .arg(m_version & 0xFF, 2, 10, QChar('0')));
    log(QString("  Files: %1").arg(m_files.join(", ")));
    log(QString("  Sample points: %1 steps").arg(m_sampleData.size()));
    log(QString("  Mode: %1").arg(m_use2BytePoints ? "DE (2-byte points)" : "DD (1-byte points)"));
    log(QString("Waiting for TCP connections..."));

    return true;
}

void VspServer::stop()
{
    if (m_serial && m_serial->isOpen()) {
        m_serial->close();
        log("Serial server stopped.");
    }
    if (m_tcpClient) {
        m_tcpClient->disconnectFromHost();
        m_tcpClient = nullptr;
    }
    if (m_tcpServer && m_tcpServer->isListening()) {
        m_tcpServer->close();
        log("TCP server stopped.");
    }
}

void VspServer::addFile(const QString& name)
{
    m_files.append(name);
}

void VspServer::addSamplePoint(uint16_t outputPin, const QVector<uint16_t>& inputPins)
{
    SampleStep step;
    step.outputPin = outputPin;
    step.inputPins = inputPins;
    m_sampleData.append(step);
}

void VspServer::setTestNgDefects(const QVector<NgDefect>& defects)
{
    m_ngDefects = defects;
}

void VspServer::onReadyRead()
{
    m_rxBuffer.append(m_serial->readAll());
    processBuffer(m_rxBuffer, m_serial);
}

void VspServer::onTcpNewConnection()
{
    QTcpSocket* client = m_tcpServer->nextPendingConnection();
    if (!client) return;

    // Only allow one client at a time (simulates single serial device)
    if (m_tcpClient) {
        log(QString("Rejecting connection from %1 (already connected)")
            .arg(client->peerAddress().toString()));
        client->disconnectFromHost();
        client->deleteLater();
        return;
    }

    m_tcpClient = client;
    m_tcpRxBuffer.clear();
    log(QString("TCP client connected: %1:%2")
        .arg(client->peerAddress().toString())
        .arg(client->peerPort()));

    connect(client, &QTcpSocket::readyRead, this, &VspServer::onTcpClientReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &VspServer::onTcpClientDisconnected);
}

void VspServer::onTcpClientReadyRead()
{
    if (!m_tcpClient) return;
    m_tcpRxBuffer.append(m_tcpClient->readAll());
    processBuffer(m_tcpRxBuffer, m_tcpClient);
}

void VspServer::onTcpClientDisconnected()
{
    if (m_tcpClient) {
        log(QString("TCP client disconnected: %1:%2")
            .arg(m_tcpClient->peerAddress().toString())
            .arg(m_tcpClient->peerPort()));
        m_tcpClient->deleteLater();
        m_tcpClient = nullptr;
        m_tcpRxBuffer.clear();
        log("Waiting for new TCP connection...");
    }
}

void VspServer::processBuffer(QByteArray& buffer, QIODevice* outputDevice)
{
    while (buffer.size() >= 2) {
        // Look for AA prefix
        int aaPos = buffer.indexOf(static_cast<char>(0xAA));
        if (aaPos < 0) {
            buffer.clear();
            return;
        }
        if (aaPos > 0) {
            buffer = buffer.mid(aaPos);
        }
        if (buffer.size() < 2) {
            return;
        }

        uint8_t cmdByte = static_cast<uint8_t>(buffer[1]);

        // Determine expected command length based on cmd byte
        int expectedLen = 2;
        switch (cmdByte) {
        case 0x01: expectedLen = 2; break;
        case 0x02: expectedLen = 2; break;
        case 0x03: expectedLen = 2; break;
        case 0x04: expectedLen = 4; break;
        case 0x05: expectedLen = 4; break;
        case 0x06:
            if (buffer.size() >= 10)
                expectedLen = 10;
            else
                expectedLen = 2;
            break;
        case 0x07:
            expectedLen = buffer.size();
            if (expectedLen > 4) expectedLen = 4;
            break;
        case 0x08: expectedLen = 3; break;
        case 0x09:
            expectedLen = buffer.size();
            if (expectedLen > 17) expectedLen = 17;
            break;
        case 0x0A: expectedLen = 2; break;
        case 0x10:
            expectedLen = buffer.size();
            if (expectedLen > 42) expectedLen = 42;
            break;
        case 0x11:
            expectedLen = buffer.size();
            if (expectedLen > 42) expectedLen = 42;
            break;
        case 0x20: expectedLen = 3; break;
        case 0x21: expectedLen = 3; break;
        case 0x22: expectedLen = 3; break;
        case 0x23: expectedLen = 3; break;
        case 0x24: expectedLen = 2; break;
        case 0x25:
            if (buffer.size() > 2)
                expectedLen = buffer.size();
            else
                expectedLen = 2;
            if (expectedLen > 28) expectedLen = 28;
            break;
        case 0x26:
            if (buffer.size() > 2)
                expectedLen = buffer.size();
            else
                expectedLen = 2;
            if (expectedLen > 30) expectedLen = 30;
            break;
        case 0x27: expectedLen = 3; break;
        case 0x28: expectedLen = 3; break;
        default:
            log(QString("Unknown command: AA %1").arg(cmdByte, 2, 16, QChar('0')));
            buffer = buffer.mid(2);
            continue;
        }

        if (buffer.size() < expectedLen) {
            return;
        }

        QByteArray cmd = buffer.left(expectedLen);
        buffer = buffer.mid(expectedLen);

        log(QString("RX: %1").arg(QString(cmd.toHex(' ').toUpper())));

        QByteArray response = handleCommand(cmd);
        if (!response.isEmpty()) {
            if (outputDevice) {
                outputDevice->write(response);
            }
            log(QString("TX: %1").arg(QString(response.toHex(' ').toUpper())));
        }
    }
}

QByteArray VspServer::handleCommand(const QByteArray& cmd)
{
    if (cmd.size() < 2 || static_cast<uint8_t>(cmd[0]) != 0xAA)
        return {};

    uint8_t cmdByte = static_cast<uint8_t>(cmd[1]);

    switch (cmdByte) {
    case 0x01:
        return cmdReadVersion();

    case 0x02:
        return cmdReadDateTime();

    case 0x03:
        return cmdReadFileCount();

    case 0x04:
        if (cmd.size() >= 4) {
            uint16_t fileNo = (static_cast<uint8_t>(cmd[2]) << 8) | static_cast<uint8_t>(cmd[3]);
            return cmdReadFileNameFixed(fileNo);
        }
        return {};

    case 0x05:
        if (cmd.size() >= 4) {
            uint16_t fileNo = (static_cast<uint8_t>(cmd[2]) << 8) | static_cast<uint8_t>(cmd[3]);
            return cmdReadFileNameVariable(fileNo);
        }
        return {};

    case 0x06:
        if (cmd.size() >= 10) {
            uint32_t pass = (static_cast<uint8_t>(cmd[2]) << 24) | (static_cast<uint8_t>(cmd[3]) << 16)
                          | (static_cast<uint8_t>(cmd[4]) << 8) | static_cast<uint8_t>(cmd[5]);
            uint32_t fail = (static_cast<uint8_t>(cmd[6]) << 24) | (static_cast<uint8_t>(cmd[7]) << 16)
                          | (static_cast<uint8_t>(cmd[8]) << 8) | static_cast<uint8_t>(cmd[9]);
            return cmdWriteTestCount(pass, fail);
        }
        return cmdReadTestCount();

    case 0x07:
        return cmdReadTestStatus(cmd.mid(2));

    case 0x08:
        if (cmd.size() >= 3)
            return cmdSimulateButton(static_cast<uint8_t>(cmd[2]));
        return {};

    case 0x09:
        return cmdUnlock(cmd.mid(2));

    case 0x0A:
        return cmdLock();

    case 0x10:
        return cmdOpenFile(cmd.mid(2));

    case 0x11:
        return cmdSaveFile(cmd.mid(2));

    case 0x20:
        if (cmd.size() >= 3)
            return cmdReadSampleData(static_cast<uint8_t>(cmd[2]));
        return {};

    case 0x21:
        if (cmd.size() >= 3)
            return cmdReadTestData(static_cast<uint8_t>(cmd[2]));
        return {};

    case 0x22:
        if (cmd.size() >= 3)
            return cmdReadNgDetail(static_cast<uint8_t>(cmd[2]));
        return {};

    case 0x23:
        if (cmd.size() >= 3)
            return cmdScanSample(static_cast<uint8_t>(cmd[2]));
        return {};

    case 0x24:
        return cmdResetSample();

    case 0x25:
        if (cmd.size() > 2)
            return cmdWriteFileOptions(cmd.mid(2));
        return cmdReadFileOptions();

    case 0x26:
        if (cmd.size() > 2)
            return cmdWriteFileOptionsV547(cmd.mid(2));
        return cmdReadFileOptionsV547();

    case 0x27:
        if (cmd.size() >= 3)
            return cmdContinuityTest(static_cast<uint8_t>(cmd[2]));
        return {};

    case 0x28:
        if (cmd.size() >= 3)
            return cmdReadPointStatus(static_cast<uint8_t>(cmd[2]));
        return {};

    default:
        log(QString("Unhandled command: %1").arg(cmdByte, 2, 16, QChar('0')));
        return {};
    }
}

// ---- Command Implementations ----

QByteArray VspServer::cmdReadVersion()
{
    // DD + version_high + version_low
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    resp.append(static_cast<char>((m_version >> 8) & 0xFF));
    resp.append(static_cast<char>(m_version & 0xFF));
    log(QString("  -> Version V%1.%2")
        .arg(m_version >> 8)
        .arg(m_version & 0xFF, 2, 10, QChar('0')));
    return resp;
}

QByteArray VspServer::cmdReadDateTime()
{
    // DD + year(2) + month + day + hour + min + sec
    QDateTime now = QDateTime::currentDateTime();
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    uint16_t year = static_cast<uint16_t>(now.date().year());
    resp.append(static_cast<char>((year >> 8) & 0xFF));
    resp.append(static_cast<char>(year & 0xFF));
    resp.append(static_cast<char>(now.date().month()));
    resp.append(static_cast<char>(now.date().day()));
    resp.append(static_cast<char>(now.time().hour()));
    resp.append(static_cast<char>(now.time().minute()));
    resp.append(static_cast<char>(now.time().second()));
    log(QString("  -> DateTime: %1").arg(now.toString("yyyy-MM-dd hh:mm:ss")));
    return resp;
}

QByteArray VspServer::cmdReadFileCount()
{
    // DD + count(2 bytes, big-endian)
    uint16_t count = static_cast<uint16_t>(m_files.size());
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    resp.append(static_cast<char>((count >> 8) & 0xFF));
    resp.append(static_cast<char>(count & 0xFF));
    log(QString("  -> FileCount: %1").arg(count));
    return resp;
}

QByteArray VspServer::cmdReadFileNameFixed(uint16_t fileNo)
{
    // DD + filename (ASCII, 40 bytes, padded with 0)
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));

    QString name;
    if (fileNo == 0) {
        // Current file
        name = m_files.isEmpty() ? "" : m_files[m_currentFileIndex];
    } else if (fileNo <= m_files.size()) {
        name = m_files[fileNo - 1];
    } else {
        name = "";
    }

    QByteArray nameBytes = name.toLatin1();
    nameBytes.resize(40, 0x00);  // Pad to 40 bytes
    resp.append(nameBytes);

    log(QString("  -> FileName[%1]: \"%2\"").arg(fileNo).arg(name));
    return resp;
}

QByteArray VspServer::cmdReadFileNameVariable(uint16_t fileNo)
{
    // DD + name_length + filename (ASCII)
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));

    QString name;
    if (fileNo == 0) {
        name = m_files.isEmpty() ? "" : m_files[m_currentFileIndex];
    } else if (fileNo <= m_files.size()) {
        name = m_files[fileNo - 1];
    } else {
        name = "";
    }

    QByteArray nameBytes = name.toLatin1();
    resp.append(static_cast<char>(nameBytes.size()));
    resp.append(nameBytes);

    log(QString("  -> FileName[%1]: len=%2, \"%3\"").arg(fileNo).arg(nameBytes.size()).arg(name));
    return resp;
}

QByteArray VspServer::cmdReadTestCount()
{
    // DD + pass(4, big-endian) + fail(4, big-endian)
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    resp.append(static_cast<char>((m_passCount >> 24) & 0xFF));
    resp.append(static_cast<char>((m_passCount >> 16) & 0xFF));
    resp.append(static_cast<char>((m_passCount >> 8) & 0xFF));
    resp.append(static_cast<char>(m_passCount & 0xFF));
    resp.append(static_cast<char>((m_failCount >> 24) & 0xFF));
    resp.append(static_cast<char>((m_failCount >> 16) & 0xFF));
    resp.append(static_cast<char>((m_failCount >> 8) & 0xFF));
    resp.append(static_cast<char>(m_failCount & 0xFF));
    log(QString("  -> TestCount: pass=%1, fail=%2").arg(m_passCount).arg(m_failCount));
    return resp;
}

QByteArray VspServer::cmdWriteTestCount(uint32_t pass, uint32_t fail)
{
    if (!m_unlocked) {
        log("  -> WriteTestCount: LOCKED, ignored (unlock first)");
    }
    m_passCount = pass;
    m_failCount = fail;
    log(QString("  -> WriteTestCount: pass=%1, fail=%2").arg(pass).arg(fail));
    return cmdReadTestCount();
}

QByteArray VspServer::cmdReadTestStatus(const QByteArray& options)
{
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));

    if (options.isEmpty()) {
        // Immediate return
        resp.append(static_cast<char>(m_testStatus));
        log(QString("  -> TestStatus: 0x%1").arg(m_testStatus, 2, 16, QChar('0')));
    } else {
        uint8_t opt = static_cast<uint8_t>(options[0]);
        if (opt >= 0xA0 && opt <= 0xA4) {
            m_autoReportOption = opt;
            resp.append(static_cast<char>(opt));
            log(QString("  -> TestStatus: auto-report option 0x%1 enabled").arg(opt, 2, 16, QChar('0')));
        } else {
            resp.append(static_cast<char>(m_testStatus));
        }
    }
    return resp;
}

QByteArray VspServer::cmdSimulateButton(uint8_t keyCode)
{
    // DD + keycode
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    resp.append(static_cast<char>(keyCode));
    log(QString("  -> SimulateButton: 0x%1").arg(keyCode, 2, 16, QChar('0')));
    return resp;
}

QByteArray VspServer::cmdUnlock(const QByteArray& password)
{
    Q_UNUSED(password);
    m_unlocked = true;
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    resp.append(static_cast<char>(0x01));  // unlocked
    log("  -> Unlock: OK");
    return resp;
}

QByteArray VspServer::cmdLock()
{
    m_unlocked = false;
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    resp.append(static_cast<char>(0x00));  // locked
    log("  -> Lock: OK");
    return resp;
}

QByteArray VspServer::cmdOpenFile(const QByteArray& filename)
{
    QString name = QString::fromLatin1(filename);
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));

    if (name.isEmpty()) {
        // Reopen current file
        resp.append(static_cast<char>(0x00));
        log("  -> OpenFile: reopen current");
    } else {
        int idx = m_files.indexOf(name);
        if (idx >= 0) {
            m_currentFileIndex = idx;
            resp.append(static_cast<char>(0x00));  // success
            log(QString("  -> OpenFile: \"%1\" OK").arg(name));
        } else {
            resp.append(static_cast<char>(0xFF));  // file not found
            log(QString("  -> OpenFile: \"%1\" NOT FOUND").arg(name));
        }
    }
    return resp;
}

QByteArray VspServer::cmdSaveFile(const QByteArray& filename)
{
    if (!m_unlocked) {
        log("  -> SaveFile: LOCKED");
    }

    QString name = QString::fromLatin1(filename);
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));

    if (name.isEmpty()) {
        resp.append(static_cast<char>(0x00));  // save current
        log("  -> SaveFile: save current OK");
    } else {
        if (m_files.contains(name)) {
            resp.append(static_cast<char>(0xFF));  // already exists
            log(QString("  -> SaveFile: \"%1\" already exists").arg(name));
        } else {
            m_files.append(name);
            resp.append(static_cast<char>(0x00));
            log(QString("  -> SaveFile: \"%1\" OK").arg(name));
        }
    }
    return resp;
}

QByteArray VspServer::buildSampleDataPacket() const
{
    // Build a packet from m_sampleData starting at m_sampleDataPos
    // Each step: pointCount(1) + outputPin(1or2) + inputPins(1or2 each)
    QByteArray data;

    for (int i = m_sampleDataPos; i < m_sampleData.size(); i++) {
        const auto& step = m_sampleData[i];
        QByteArray stepData;

        uint8_t totalPoints = static_cast<uint8_t>(1 + step.inputPins.size());
        stepData.append(static_cast<char>(totalPoints));

        if (m_use2BytePoints) {
            stepData.append(static_cast<char>((step.outputPin >> 8) & 0xFF));
            stepData.append(static_cast<char>(step.outputPin & 0xFF));
            for (auto pin : step.inputPins) {
                stepData.append(static_cast<char>((pin >> 8) & 0xFF));
                stepData.append(static_cast<char>(pin & 0xFF));
            }
        } else {
            stepData.append(static_cast<char>(step.outputPin & 0xFF));
            for (auto pin : step.inputPins) {
                stepData.append(static_cast<char>(pin & 0xFF));
            }
        }

        if (data.size() + stepData.size() > 255) {
            break;  // Would exceed 255 bytes
        }
        data.append(stepData);
    }

    return data;
}

QByteArray VspServer::cmdReadSampleData(uint8_t position)
{
    QByteArray resp;
    resp.append(static_cast<char>(m_use2BytePoints ? 0xDE : 0xDD));

    if (position == 0x00) {
        m_sampleDataPos = 0;
    } else if (position == 0xFF) {
        // Resend last packet, don't change position
    }
    // position == 0x01: continue from current

    if (m_sampleDataPos >= m_sampleData.size()) {
        // No data
        log("  -> ReadSampleData: no data");
        return resp;
    }

    QByteArray packet = buildSampleDataPacket();
    resp.append(packet);

    // Advance position
    // Calculate how many steps were included
    int pos = 0;
    int stepsIncluded = 0;
    for (int i = m_sampleDataPos; i < m_sampleData.size(); i++) {
        const auto& step = m_sampleData[i];
        int stepSize = 1;  // point count byte
        if (m_use2BytePoints) {
            stepSize += 2 + step.inputPins.size() * 2;
        } else {
            stepSize += 1 + step.inputPins.size();
        }
        if (pos + stepSize > 255) break;
        pos += stepSize;
        stepsIncluded++;
    }
    if (position != 0xFF) {
        m_sampleDataPos += stepsIncluded;
    }

    log(QString("  -> ReadSampleData[pos=0x%1]: %2 bytes, %3 steps")
        .arg(position, 2, 16, QChar('0'))
        .arg(packet.size())
        .arg(stepsIncluded));
    return resp;
}

QByteArray VspServer::cmdReadTestData(uint8_t position)
{
    // Same format as sample data - in a real device, this reflects live test state
    log(QString("  -> ReadTestData (same as sample data)"));
    return cmdReadSampleData(position);
}

QByteArray VspServer::cmdReadNgDetail(uint8_t position)
{
    QByteArray resp;
    resp.append(static_cast<char>(m_use2BytePoints ? 0xDE : 0xDD));

    if (position == 0x00) {
        m_ngDataPos = 0;
    }

    if (m_ngDefects.isEmpty() || m_ngDataPos >= m_ngDefects.size()) {
        log("  -> ReadNgDetail: no defects");
        return resp;
    }

    QByteArray data;
    int included = 0;
    for (int i = m_ngDataPos; i < m_ngDefects.size(); i++) {
        const auto& defect = m_ngDefects[i];
        QByteArray defectData;
        defectData.append(static_cast<char>(defect.code));

        if (m_use2BytePoints) {
            defectData.append(static_cast<char>((defect.pinA >> 8) & 0xFF));
            defectData.append(static_cast<char>(defect.pinA & 0xFF));
            defectData.append(static_cast<char>((defect.pinB >> 8) & 0xFF));
            defectData.append(static_cast<char>(defect.pinB & 0xFF));
        } else {
            defectData.append(static_cast<char>(defect.pinA & 0xFF));
            defectData.append(static_cast<char>(defect.pinB & 0xFF));
        }

        if (data.size() + defectData.size() > 255) break;
        data.append(defectData);
        included++;
    }

    if (position != 0xFF) {
        m_ngDataPos += included;
    }

    resp.append(data);
    log(QString("  -> ReadNgDetail[pos=0x%1]: %2 defects").arg(position, 2, 16, QChar('0')).arg(included));
    return resp;
}

QByteArray VspServer::cmdScanSample(uint8_t seekCount)
{
    Q_UNUSED(seekCount);
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    resp.append(static_cast<char>(0x00));  // success
    log(QString("  -> ScanSample: seekCount=%1, OK").arg(seekCount));
    return resp;
}

QByteArray VspServer::cmdResetSample()
{
    m_sampleData.clear();
    m_sampleDataPos = 0;
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    log("  -> ResetSample: OK");
    return resp;
}

QByteArray VspServer::cmdReadFileOptions()
{
    // DD + 26 bytes
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    resp.append(m_fileOptions);
    log(QString("  -> ReadFileOptions: %1 bytes").arg(m_fileOptions.size()));
    return resp;
}

QByteArray VspServer::cmdWriteFileOptions(const QByteArray& data)
{
    if (!m_unlocked) {
        log("  -> WriteFileOptions: LOCKED");
    }
    // Overwrite the first N bytes of file options
    int len = qMin(data.size(), m_fileOptions.size());
    for (int i = 0; i < len; i++) {
        m_fileOptions[i] = data[i];
    }
    log(QString("  -> WriteFileOptions: wrote %1 bytes").arg(len));
    return cmdReadFileOptions();
}

QByteArray VspServer::cmdReadFileOptionsV547()
{
    // DD + 28 bytes
    QByteArray resp;
    resp.append(static_cast<char>(0xDD));
    resp.append(m_fileOptionsV547);
    log(QString("  -> ReadFileOptionsV547: %1 bytes").arg(m_fileOptionsV547.size()));
    return resp;
}

QByteArray VspServer::cmdWriteFileOptionsV547(const QByteArray& data)
{
    if (!m_unlocked) {
        log("  -> WriteFileOptionsV547: LOCKED");
    }
    int len = qMin(data.size(), m_fileOptionsV547.size());
    for (int i = 0; i < len; i++) {
        m_fileOptionsV547[i] = data[i];
    }
    log(QString("  -> WriteFileOptionsV547: wrote %1 bytes").arg(len));
    return cmdReadFileOptionsV547();
}

QByteArray VspServer::cmdContinuityTest(uint8_t position)
{
    // Same as ReadSampleData but triggers a test first
    log("  -> ContinuityTest: performing test...");

    // Simulate: increment pass or fail count
    if (m_ngDefects.isEmpty()) {
        m_passCount++;
        m_testStatus = 0x11;  // wire inserted, OK
    } else {
        m_failCount++;
        m_testStatus = 0x12;  // wire inserted, NG
    }

    return cmdReadSampleData(position);
}

QByteArray VspServer::cmdReadPointStatus(uint8_t position)
{
    QByteArray resp;
    resp.append(static_cast<char>(m_use2BytePoints ? 0xDE : 0xDD));

    if (position == 0x00 || position == 0x01) {
        for (auto pin : m_lowPins) {
            if (m_use2BytePoints) {
                resp.append(static_cast<char>((pin >> 8) & 0xFF));
                resp.append(static_cast<char>(pin & 0xFF));
            } else {
                resp.append(static_cast<char>(pin & 0xFF));
            }
        }
    }

    log(QString("  -> ReadPointStatus: %1 low pins").arg(m_lowPins.size()));
    return resp;
}

void VspServer::log(const QString& msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString fullMsg = QString("[%1] %2").arg(timestamp, msg);
    qDebug().noquote() << fullMsg;
    emit logMessage(fullMsg);
}

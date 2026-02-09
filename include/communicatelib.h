#ifndef COMMUNICATELIB_H
#define COMMUNICATELIB_H

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QHostAddress>
#include <QTcpSocket>
#include <QDateTime>
#include <memory>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QDebug>

namespace CommunicateClass
{

class CommunicateInterface : public QObject
{
public:
    virtual bool init() = 0;
    virtual void communicate(QString) = 0;
    virtual bool communicate(QString, QString&) = 0;
    virtual bool communicate(QByteArray, QByteArray&) = 0;
};

class TcpClient : public CommunicateInterface
{
public:
    TcpClient(QString ip, int port) : mIp(ip), mPort(port)
    {
        mpQHostAddress = new QHostAddress();
        mpTcpSocket = new QTcpSocket();
    }

    ~TcpClient()
    {
        delete mpTcpSocket;
        delete mpQHostAddress;
    }

    bool init() override
    {
        if (!mpQHostAddress->setAddress(mIp))
        {
            qDebug() << mIp << "server ip address error!";
            return false;
        }

        mpTcpSocket->connectToHost(*mpQHostAddress, mPort);

        if (false == mpTcpSocket->waitForConnected(1000))
        {
            qDebug() << mIp << "connect failed.";
            return false;
        }

        qDebug() << mIp << "connect ok.";
        mbInit = true;
        return true;
    }

    void disConnect()
    {
        mpTcpSocket->disconnectFromHost();
    }

    void send(const QByteArray msg)
    {
        mRecData.clear();
        qDebug() << "";
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") << mIp << " send:" << msg << "\r";
        mpTcpSocket->write(msg);
    }

    void send(const QString msg)
    {
        mRecData.clear();
        qDebug() << "";
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") << mIp << " send:" << msg << "\r";
        mpTcpSocket->write(msg.toLatin1(), msg.length());
    }

    bool getRec()
    {
        if (false == this->mpTcpSocket->waitForReadyRead())
        {
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz");
            qDebug() << "Server does not response!";
            return false;
        }

        QByteArray datagram = mpTcpSocket->readAll();
        QString msg = datagram.data();
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") << msg;

        return true;
    }

    bool getRec(QByteArray& msg)
    {
        if (false == this->mpTcpSocket->waitForReadyRead())
        {
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz");
            qDebug() << "Server does not response!";
            return false;
        }

        QByteArray datagram = mpTcpSocket->readAll();
        msg = datagram;
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") << "recv: " << msg;

        return true;
    }

    bool getRec(QString& msg)
    {
        if (false == this->mpTcpSocket->waitForReadyRead())
        {
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz");
            qDebug() << "Server does not response!";
            return false;
        }

        QByteArray datagram = mpTcpSocket->readAll();
        msg = datagram.data();
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") << msg;

        return true;
    }

    void communicate(QString tSend) override
    {
        send(tSend);
    }

    bool communicate(const QString tSend, QString& mRecv) override
    {
        if (!init()) {
            qDebug() << "mbInit:false";
            return false;
        }

        send(tSend);

        if (false == getRec(mRecv)) {
            return false;
        }

        disConnect();
        return true;
    }

    bool communicate(QByteArray tSend, QByteArray& mRecv) override
    {
        send(tSend);

        if (false == getRec(mRecv)) {
            return false;
        }

        return true;
    }

    QTcpSocket *mpTcpSocket;

private:
    bool mbInit = false;
    QString mIp;
    QString userName;
    int mPort;
    QHostAddress *mpQHostAddress;
    QString mRecData;
};

class ComPortOne : public CommunicateInterface
{
    Q_OBJECT

public:
    ComPortOne(unsigned int portNo, int baudRate = 115200, int breakLength = 256, int breakDelay = 1000)
        : mPortNo(portNo), mBaudRate(baudRate), mBreakLength(breakLength), mBreakDelay(breakDelay)
    {
        mpSerial->setPortName(QString("COM%1").arg(portNo));
        mpSerial->setBaudRate(baudRate);
        mpSerial->setParity(QSerialPort::EvenParity);
        mpSerial->setDataBits(QSerialPort::Data8);
        mpSerial->setStopBits(QSerialPort::OneStop);
        mpSerial->setFlowControl(QSerialPort::NoFlowControl);
    }

    ~ComPortOne()
    {
        mpSerial->close();
    }

    void close()
    {
        qDebug() << "close com" << mPortNo << "\r";
        mpSerial->close();
    }

    bool init() override
    {
        mpSerial->close();

        if (mpSerial->open(QIODevice::ReadWrite))
        {
            qDebug() << "open com" << mPortNo << " success" << "\r";
            mbInit = true;
            return true;
        }
        else
        {
            qDebug() << "open com" << mPortNo << " failed" << "\r";
            return false;
        }
    }

    qint64 send(const QString sendData)
    {
        mRecData.clear();
        mpSerial->clear();
        QByteArray tSend = QByteArray::fromHex(sendData.toLocal8Bit());

        qDebug() << "";
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") << mPortNo << "send:" << tSend;

        return mpSerial->write(tSend);
    }

    inline void recData()
    {
        mRecData.clear();
        QElapsedTimer timer;
        timer.start();

        forever
        {
            if (mRecData.length() > mBreakLength || timer.elapsed() > mBreakDelay)
            {
                break;
            }

            while (mpSerial->waitForReadyRead(10))
            {
                QByteArray tRec = mpSerial->readAll();
                mRecData += tRec.data();
            }
        }

        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") << mPortNo << "rec:" << mRecData;
    }

    QString getRec()
    {
        return mRecData;
    }

    void communicate(const QString tSend) override
    {
        if (!mbInit) {
            qDebug() << "mbInit:false";
            return;
        }
        send(tSend);
    }

    bool communicate(const QString tSend, QString& mRecv) override
    {
        if (!mbInit) {
            qDebug() << "mbInit:false";
            return false;
        }

        auto a = send(tSend);
        qDebug() << "send success?" << a << "\r";

        recData();
        mRecv = mRecData;

        return true;
    }

    bool communicate(const QByteArray tSend, QByteArray& tRecv) override
    {
        if (!mbInit) {
            qDebug() << "mbInit:false";
            return false;
        }

        qDebug() << "";
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") << mPortNo << "send:" << tSend.toHex();
        mpSerial->write(tSend);

        QElapsedTimer timer;
        timer.start();
        tRecv.clear();

        forever
        {
            if (tRecv.size() >= mBreakLength || timer.elapsed() > mBreakDelay)
            {
                break;
            }

            QByteArray tRec = mpSerial->readAll();
            tRecv += tRec;

            QEventLoop loop;
            QTimer::singleShot(5, &loop, &QEventLoop::quit);
            loop.exec();
        }

        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") << mPortNo << "rec:" << tRecv.toHex();
        return true;
    }

public slots:
    void recvMsg()
    {
        mRecData.clear();
        QByteArray tRec = mpSerial->readAll();
        mRecData += tRec.data();
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") << mPortNo << "rec:" << mRecData;
    }

private:
    bool mbInit = false;
    unsigned int mPortNo;
    int mBaudRate;
    std::shared_ptr<QSerialPort> mpSerial = std::make_shared<QSerialPort>();
    QString mRecData;
    int mBreakLength;
    int mBreakDelay;
};

}  // namespace CommunicateClass

#endif // COMMUNICATELIB_H

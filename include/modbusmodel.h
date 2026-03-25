#ifndef MODBUSMODEL_H
#define MODBUSMODEL_H

#ifdef HAVE_SERIALBUS

#include <QModbusDataUnit>
#include <QModbusTcpClient>
#include <QModbusReply>
#include <QDebug>

namespace ModbusHelper
{

class ModbusModel : public QObject
{
    Q_OBJECT

public:
    explicit ModbusModel(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    void setDevice(std::shared_ptr<QModbusClient> device)
    {
        m_device = device;
    }

    bool readHoldingRegisters(int serverAddress, int startAddress, quint16 numberOfEntries)
    {
        if (!m_device || m_device->state() != QModbusDevice::ConnectedState) {
            qDebug() << "Device not connected";
            return false;
        }

        QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, startAddress, numberOfEntries);

        if (auto* reply = m_device->sendReadRequest(readUnit, serverAddress)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, [this, reply]() {
                    if (reply->error() == QModbusDevice::NoError) {
                        const QModbusDataUnit unit = reply->result();
                        emit valuesReceived(unit.values().toList());
                    } else {
                        qDebug() << "Modbus read error:" << reply->errorString();
                    }
                    reply->deleteLater();
                });
            } else {
                delete reply;
            }
            return true;
        }

        return false;
    }

    bool writeHoldingRegister(int serverAddress, int address, quint16 value)
    {
        if (!m_device || m_device->state() != QModbusDevice::ConnectedState) {
            qDebug() << "Device not connected";
            return false;
        }

        QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, address, 1);
        writeUnit.setValue(0, value);

        if (auto* reply = m_device->sendWriteRequest(writeUnit, serverAddress)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, [reply]() {
                    if (reply->error() != QModbusDevice::NoError) {
                        qDebug() << "Modbus write error:" << reply->errorString();
                    }
                    reply->deleteLater();
                });
            } else {
                delete reply;
            }
            return true;
        }

        return false;
    }

signals:
    void valuesReceived(const QList<quint16>& values);

private:
    std::shared_ptr<QModbusClient> m_device;
};

}  // namespace ModbusHelper

#endif // HAVE_SERIALBUS

#endif // MODBUSMODEL_H

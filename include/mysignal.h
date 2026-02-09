#ifndef MYSIGNAL_H
#define MYSIGNAL_H

#include <QObject>
#include <QMessageBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QTextBrowser>
#include <QTableWidgetItem>
#include <QProgressBar>
#include <QDialog>

namespace Drose
{

class MySignalUi : public QObject
{
    Q_OBJECT

public:
    MySignalUi()
    {
        connect(this, &MySignalUi::proBarSignal, this, &MySignalUi::proBarSlot);
        connect(this, QOverload<QPushButton*, const QString>::of(&MySignalUi::colorSignal),
                this, QOverload<QPushButton*, const QString>::of(&MySignalUi::colorSlot));
        connect(this, QOverload<QCheckBox*, const QString>::of(&MySignalUi::colorSignal),
                this, QOverload<QCheckBox*, const QString>::of(&MySignalUi::colorSlot));
        connect(this, QOverload<QLabel*, const QString>::of(&MySignalUi::colorSignal),
                this, QOverload<QLabel*, const QString>::of(&MySignalUi::colorSlot));
        connect(this, QOverload<QTableWidgetItem*, const QColor, int>::of(&MySignalUi::colorSignal),
                this, QOverload<QTableWidgetItem*, const QColor>::of(&MySignalUi::colorSlot));
        connect(this, &MySignalUi::ableSignal, this, &MySignalUi::ableSlot);
        connect(this, &MySignalUi::showMsgSignal, this, &MySignalUi::showMsg);
        connect(this, &MySignalUi::showDialogSignal, this, &MySignalUi::showDialog);
        connect(this, &MySignalUi::textSignal, this, &MySignalUi::textSlot);
    }

signals:
    void proBarSignal(QProgressBar*, int);
    void colorSignal(QPushButton*, const QString);
    void ableSignal(QPushButton*, bool);
    void colorSignal(QCheckBox*, const QString);
    void colorSignal(QLabel*, const QString);
    void colorSignal(QTableWidgetItem*, const QColor, int);
    void showMsgSignal(QTextBrowser*, const QString&);
    void showDialogSignal(const QString&);
    void textSignal(QTableWidgetItem*, const QString);

private slots:
    void proBarSlot(QProgressBar* p, int t)
    {
        p->setValue(t);
    }

    void colorSlot(QPushButton* p, const QString t)
    {
        p->setStyleSheet(t);
    }

    void ableSlot(QPushButton* p, bool b)
    {
        p->setEnabled(b);
    }

    void colorSlot(QCheckBox* p, const QString t)
    {
        p->setStyleSheet(t);
    }

    void colorSlot(QLabel* p, const QString t)
    {
        p->setStyleSheet(t);
    }

    void colorSlot(QTableWidgetItem* p, const QColor t)
    {
        p->setBackground(t);
    }

    void showMsg(QTextBrowser* p, const QString t)
    {
        p->append(t);
    }

    void showDialog(const QString& msg)
    {
        QMessageBox::information(&mDialog, "", msg);
    }

    void textSlot(QTableWidgetItem* p, const QString t)
    {
        if (p) p->setText(t);
    }

private:
    QDialog mDialog;
};

}  // namespace Drose

#endif // MYSIGNAL_H

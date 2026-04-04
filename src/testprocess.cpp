#include "qiotest.h"
#include <QMessageBox>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTextCodec>

static QString csvEscape(QString s)
{
    const QChar comma(',');
    const QChar quote('"');
    const QChar lf('\n');
    const QChar cr('\r');

    const bool needsQuotes = s.contains(comma) || s.contains(quote) || s.contains(lf) || s.contains(cr);
    if (s.contains(quote))
        s.replace(quote, QStringLiteral("\"\""));
    return needsQuotes ? (QStringLiteral("\"") + s + QStringLiteral("\"")) : s;
}

bool QIoTest::saveTestDataFile(bool isStepTest, bool overallOk)
{
    // Output: .\data\<orderNo>.csv next to exe; same order appends to one file
    QDir exeDir(gExePath);
    const QString dataRoot = exeDir.filePath("data");

    const QString orderNo = QFileInfo(inputFile_).completeBaseName();
    QString orderFileBase = orderNo;
    if (orderFileBase.isEmpty())
        orderFileBase = QStringLiteral("_empty");
    for (const QChar c : QStringLiteral("\\/:*?\"<>|"))
        orderFileBase.replace(c, QLatin1Char('_'));

    if (!QDir().mkpath(dataRoot))
    {
        qDebug() << "Failed to create data dir:" << dataRoot;
        return false;
    }

    const QString outPath = QDir(dataRoot).filePath(orderFileBase + QStringLiteral(".csv"));
    const bool fileExists = QFile::exists(outPath);

    QFile f(outPath);
    const QIODevice::OpenMode mode = fileExists
        ? (QIODevice::WriteOnly | QIODevice::Append)
        : (QIODevice::WriteOnly | QIODevice::Truncate);
    if (!f.open(mode))
    {
        qDebug() << "Failed to open csv:" << outPath << f.errorString();
        return false;
    }

    QTextStream tsOut(&f);
    tsOut.setCodec("UTF-8");

    if (!fileExists)
    {
        // UTF-8 BOM for Excel/Notepad on Windows (new file only)
        f.write("\xEF\xBB\xBF");
        tsOut
            << csvEscape(QStringLiteral("年月日")) << ','
            << csvEscape(QStringLiteral("时间")) << ','
            << csvEscape(QStringLiteral("订单号")) << ','
            << csvEscape(QStringLiteral("测试结果")) << ','
            << csvEscape(QStringLiteral("NG针点")) << ','
            << csvEscape(QStringLiteral("备注")) << "\r\n";
    }

    const auto now = QDateTime::currentDateTime();
    const QString dateStr = now.date().toString("yyyy/M/d");
    const QString timeStr = now.time().toString("HH:mm");

    if (overallOk)
    {
        const QString summary = QStringLiteral("结果：OK 单步测试=%0").arg(isStepTest ? 1 : 0);
        tsOut << csvEscape(dateStr) << ',' << csvEscape(timeStr) << ',' << csvEscape(orderNo) << ','
              << csvEscape(summary) << ",,\r\n";
    }
    else
    {
        const QString summary = QStringLiteral("结果：NG 失败=%0").arg(ng_list_.count());
        tsOut << csvEscape(dateStr) << ',' << csvEscape(timeStr) << ',' << csvEscape(orderNo) << ','
              << csvEscape(summary) << ",,\r\n";

        for (const auto& item : ng_list_)
        {
            const QString coor = item.coordinateL + QStringLiteral("→") + item.coordinateR;
            const QString pins = item.pinL + QStringLiteral("→") + item.pinR;
            tsOut << csvEscape(dateStr) << ',' << csvEscape(timeStr) << ',' << csvEscape(coor) << ','
                  << csvEscape(item.category) << ',' << csvEscape(pins) << ",\r\n";
        }
    }

    tsOut.flush();
    f.close();

    qDebug() << "Saved test data csv:" << outPath;
    return true;
}

bool QIoTest::checkShort(QSet<int> item, int L, int R)
{
    for (auto&& set : testTaskSets)
    {
        if (set.contains(L) && set.contains(R))
        {
            item.remove(L);
            item.remove(R);

            if (!set.contains(item))  // scan group larger than expected test pair set
            {
                lineMsg_ = QObject::tr("短路:") + QString("%0 - %1").arg(L).arg(R);
                for (const auto& pin : item)
                {
                    lineMsg_ += QString(",%0").arg(pin);
                }

                return false;  // short
            }
        }
    }

    return true;
}

bool QIoTest::checkPins(itemTest item)
{
    lineMsg_.clear();

    int L = item.pinL.toInt();
    int R = item.pinR.toInt();

    int index;
    for (index = 0; index < pinLinkGroups.length(); index++)
    {
        auto&& group = pinLinkGroups[index];

        if (group.contains(L) && group.contains(R))
        {
            if (enShort_ && !checkShort(group, L, R))
            {
                return false;  // short
            }

            break;
        }
    }

    if (index >= pinLinkGroups.length())
    {
        lineMsg_ = QString("NG: %0 - %1").arg(L).arg(R);
        return false;  // disconnect
    }

    return true;
}

bool QIoTest::selfCheck()
{
    for (const auto& item : com_pairs_)
    {
        auto val_p = std::vector<uint16_t>(item.begin(), item.end());
        if (val_p.size() != 2)
        {
            qDebug() << QString("error: val_p.size()") << "\r";
            return false;
        }
    }

    return true;
}

bool QIoTest::lineTest(itemTest item)
{
    if (!checkPins(item))
    {
        return false;
    }

    return true;
}

void QIoTest::slotStartList()
{
    ui.labelResult->clear();
    if (mListTest.size() < 1)
    {
        QMessageBox::information(this, "", QObject::tr("请先加载测试档案！"));
        gpSignal->colorSignal(gpUi->pushButtonStart, "QPushButton{background:}");
        ui.pushButtonStart->setEnabled(true);
        return;
    }

    bool result = true;

    ng_list_.clear();
    gpUi->tableWidgetNg->clear();
    for (auto it = mListTest.begin(); it != mListTest.end(); ++it)
    {
        QApplication::processEvents();

        if (mbExit)
        {
            mbExit = false;
            break;
        }

        if (mbPause)
        {
            it--;
            continue;
        }

        if (!mCurCategorys.contains(it->category))
        {
            it->bInlist = false;
            continue;
        }

        ui.tableWidget->selectRow(it->rowNo);
        ui.tableWidget->setFocus();

        ui.labelCoordinateL->setText(it->coordinateL);
        ui.labelCoordinateR->setText(it->coordinateR);
        ui.labelPinL->setText(it->pinL);
        ui.labelPinR->setText(it->pinR);
        ui.labelCategory->setText(it->category);

        if (!lineTest(*it))
        {
            result = false;

            gpSignal->textSignal(ui.tableWidget->item(it->rowNo, 0), "  NG  ");
            gpSignal->colorSignal(ui.tableWidget->item(it->rowNo, 0), QColor(255, 0, 0), 0);

            ui.labelResult->setText(QStringLiteral("<font style='font-size:40px; color:red;'>%0</font>").arg(lineMsg_));

            ng_list_.append(*it);
        }
        else
        {
            ui.tableWidget->item(it->rowNo, 0)->setText("  OK  ");
            ui.tableWidget->item(it->rowNo, 0)->setBackground(QColor(0, 255, 0));
        }
    }

    // NG list show
    gpUi->tableWidgetNg->setRowCount(ng_list_.count());
    gpUi->tableWidgetNg->setColumnCount(10);
    for (int i = 0; i < ng_list_.count(); i++)
    {
        int index = 0;

        gpUi->tableWidgetNg->setItem(i, index++, new QTableWidgetItem("  NG  "));
        ui.tableWidgetNg->item(i, 0)->setForeground(QColor(255, 0, 0));
        gpUi->tableWidgetNg->setItem(i, index++, new QTableWidgetItem(ng_list_[i].coordinateL));
        gpUi->tableWidgetNg->setItem(i, index++, new QTableWidgetItem(ng_list_[i].coordinateR));
        gpUi->tableWidgetNg->setItem(i, index++, new QTableWidgetItem(ng_list_[i].category));
        gpUi->tableWidgetNg->setItem(i, index++, new QTableWidgetItem(""));
        gpUi->tableWidgetNg->setItem(i, index++, new QTableWidgetItem(""));
        gpUi->tableWidgetNg->setItem(i, index++, new QTableWidgetItem(""));
        gpUi->tableWidgetNg->setItem(i, index++, new QTableWidgetItem(""));
        gpUi->tableWidgetNg->setItem(i, index++, new QTableWidgetItem(ng_list_[i].pinL));
        gpUi->tableWidgetNg->setItem(i, index++, new QTableWidgetItem(ng_list_[i].pinR));
    }
    gpUi->tableWidgetNg->resizeColumnsToContents();

    if (result)
    {
        ui.labelResult->setText(QStringLiteral("<font style='font-size:40px; color:green;'>OK</font>"));
    }

    gpUi->tableWidget->resizeColumnsToContents();
    gpSignal->colorSignal(gpUi->pushButtonStart, "QPushButton{background:}");
    ui.pushButtonStart->setEnabled(true);

    // Save test data to .\data (filename derived from inputFile_)
    saveTestDataFile(false, result);
}

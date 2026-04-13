#include "qiotest.h"
#include <QModelIndex>
#include <QModelIndexList>
#include <QItemSelectionModel>
#include <vector>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSoundEffect>

GLOBAL

using namespace std;
using namespace Drose;

QIoTest::QIoTest(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    this->setWindowTitle(QStringLiteral("导通检测仪  本机扫描总点数=1024  软件版本v4.0.5  东莞精伟智能"));

    gpUi = &ui;

    gpSignal = std::make_shared<Drose::MySignalUi>();

    gExePath = QCoreApplication::applicationDirPath();

    gpUi->tabWidget->setCurrentIndex(0);
    gpUi->tableWidget->horizontalHeader()->setVisible(false);
    gpUi->tableWidget->verticalHeader()->setVisible(false);
    gpUi->tableWidgetNg->horizontalHeader()->setVisible(false);
    gpUi->tableWidgetNg->verticalHeader()->setVisible(false);

    // COM port init
    for (size_t i = 0; i < 10; i++)
    {
        gpUi->comboBox->addItem(QString("Com %0").arg(i));
    }
    auto com = settings.value("com_port").toInt();
    gpUi->comboBox->setCurrentIndex(com);
    gpComClient = std::make_shared<CommunicateClass::ComPortOne>(com);
    gpComClient->init();

    // Load mapping table (duizhao)
    try
    {
#ifdef HAVE_QXLSX
        const auto mappingPath = gExePath + "/cfg/duizhao.xlsx";
        if (QFileInfo::exists(mappingPath)) {
            FileIo::XlsxFile file;
            file.readExcel(mappingPath, num_line_map_);
        } else {
            qDebug() << "Mapping file missing:" << mappingPath;
        }
#else
        qDebug() << "QXlsx not available, skipping Excel file load";
#endif
    }
    catch (...)
    {
        qDebug() << "Read io failed...";
    }

    // COM port selection
    connect(ui.comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        settings.setValue("com_port", index);
    });

    connect(ui.pushButtonMoveUp, &QPushButton::clicked, [this]() {
        QModelIndexList indexes = ui.listWidgetDown->selectionModel()->selectedRows();

        if (indexes.size() > 0)
        {
            auto tItem = ui.listWidgetDown->takeItem(indexes.at(0).row());
            ui.listWidgetUp->addItem(tItem);
            mCurCategorys.insert(tItem->text());
        }
    });

    connect(ui.pushButtonMoveDown, &QPushButton::clicked, [this]() {
        QModelIndexList indexes = ui.listWidgetUp->selectionModel()->selectedRows();

        if (indexes.size() > 0)
        {
            auto tItem = ui.listWidgetUp->takeItem(indexes.at(0).row());
            ui.listWidgetDown->addItem(tItem);
            mCurCategorys.remove(tItem->text());
        }
    });

    connect(ui.pushButtonMoveUp_a, &QPushButton::clicked, [this]() {
        while (ui.listWidgetDown->count() > 0)
        {
            auto tItem = ui.listWidgetDown->takeItem(0);
            ui.listWidgetUp->addItem(tItem);
            mCurCategorys.insert(tItem->text());
        }
    });

    connect(ui.pushButtonMoveDown_a, &QPushButton::clicked, [this]() {
        while (ui.listWidgetUp->count() > 0)
        {
            auto tItem = ui.listWidgetUp->takeItem(0);
            ui.listWidgetDown->addItem(tItem);
            mCurCategorys.remove(tItem->text());
        }
    });

    // Select input file
    auto input = settings.value("input_path").toString();
    ui.lineEdit_input->setText(input);
    inputFile_ = input;
    connect(ui.pushButtonSelect, &QPushButton::clicked, [this]() {
        auto path = QFileDialog::getOpenFileName(nullptr, QStringLiteral("选择图号"),
            QFileInfo(inputFile_).path(), QString("*.xlsx"));

        if (path.isEmpty())
            return;

        ui.lineEdit_input->setText(path);
        inputFile_ = path;
        settings.setValue("input_path", path);
    });

    connect(ui.pushButton_input, &QPushButton::clicked, [this]() {
        QFileInfo fileInfo(inputFile_);
        if (!fileInfo.exists()) {
            QMessageBox::information(this, "", QStringLiteral("选择图号错误!"));
            qDebug() << "Input file error: " << inputFile_;
            return;
        }

#ifdef HAVE_QXLSX
        // Read input
        QStringList input_list;
        try
        {
            FileIo::XlsxFile file;
            file.readExcel(inputFile_, 2, input_list);
        }
        catch (...)
        {
            qDebug() << "input.readExcel failed...";
            return;
        }

        QRegularExpression regex("_\\d+_[A-Z]");

        for (auto& item : input_list)
        {
            QRegularExpressionMatch match = regex.match(item);

            if (match.hasMatch())
            {
                QString matched = match.captured(0);
                item = matched.remove(0, 1);
                qDebug() << "Input: " << item;
            }
        }

        // Mapping (duizhao)
        for (auto& item : input_list)
        {
            for (size_t i = 0; i < num_line_map_.size(); i++)
            {
                if (item == num_line_map_[i][0])
                {
                    item = num_line_map_[i][1];
                    break;
                }
            }
        }

        // Load to UI
        ui.listWidgetUp->clear();
        ui.listWidgetDown->clear();

        std::vector<QString> selected;
        for (auto it = mCurCategorys.begin(); it != mCurCategorys.end(); ++it)
        {
            selected.push_back(*it);
        }

        for (const auto& item : selected)
        {
            if (input_list.contains(item))
            {
                ui.listWidgetUp->addItem(item);
            }
            else
            {
                ui.listWidgetDown->addItem(item);
                mCurCategorys.remove(item);
            }
        }
#endif

        QMessageBox::information(this, "", QStringLiteral("OK!"));
    });

    // Load test file
    auto path = settings.value("file_path").toString();
    ui.lineEdit_path->setText(path);
    mFilePath = path;
    connect(ui.pushButtonOpenFile, &QPushButton::clicked, [this]() {
        auto path = QFileDialog::getOpenFileName(nullptr, QStringLiteral("打开文件"),
            QFileInfo(mFilePath).path(), QString("*.xlsx"));

        if (path.isEmpty())
            return;

        ui.lineEdit_path->setText(path);
        mFilePath = path;
        settings.setValue("file_path", path);
    });

    connect(ui.pushButton_load, &QPushButton::clicked, [this]() {
        QFileInfo fileInfo(mFilePath);
        if (!fileInfo.exists()) {
            QMessageBox::information(this, "", QStringLiteral("文件路径错误!"));
            qDebug() << "File exists error: " << mFilePath;
            return;
        }

#ifdef HAVE_QXLSX
        // Read Excel
        try
        {
            FileIo::XlsxFile file;
            file.readExcel(mFilePath, ui.tableWidget);
        }
        catch (...)
        {
            qDebug() << "Read io failed...";
            return;
        }
#endif

        // Load UI
        gpUi->tableWidget->resizeColumnsToContents();

        auto count = ui.tableWidget->rowCount();

        mListTest.clear();
        mCurCategorys.clear();

        for (int i = 0; i < count; i++)
        {
            auto safeText = [&](int row, int col) -> QString {
                auto *item = ui.tableWidget->item(row, col);
                return item ? item->text() : QString();
            };

            itemTest tItem;
            tItem.bInlist = true;
            tItem.result = -1;
            tItem.rowNo = i;
            tItem.coordinateL = safeText(i, 1);
            tItem.coordinateR = safeText(i, 2);
            tItem.category = safeText(i, 3);
            tItem.pinL = safeText(i, 8);
            tItem.pinR = safeText(i, 9);
            mListTest.push_back(tItem);

            if (!tItem.category.isEmpty())
                mCurCategorys.insert(tItem.category);
        }

        QMessageBox::information(this, "", QStringLiteral("OK!"));
    });

    connect(ui.pushButtonEdit, &QPushButton::clicked, [this]() {
        static bool b = false;
        b = !b;
        ui.pushButtonSave->setEnabled(b);
    });

    ui.pushButtonSave->setEnabled(false);
    connect(ui.pushButtonSave, &QPushButton::clicked, [this]() {
#ifdef HAVE_QXLSX
        FileIo::XlsxFile file;
        file.writeExcel(mFilePath, ui.tableWidget);
#endif
    });

    connect(ui.pushButtonSelfCheck, &QPushButton::clicked, [this]() {
        selfCheck();
    });

    connect(ui.pushButtonLockScreen, &QPushButton::clicked, [this]() {
        QDialog tDialog;
        QLabel tLabel("Lock");
        QFont ft;
        ft.setPointSize(80);
        tLabel.setFont(ft);

        QVBoxLayout tpLayout;
        tDialog.setFixedSize(500, 400);
        tLabel.setFixedSize(300, 200);
        tpLayout.addWidget(&tLabel);
        tDialog.setLayout(&tpLayout);
        tDialog.exec();
    });

    // Find coordinate
    connect(ui.btn_find_coor, &QPushButton::clicked, [this]() {
        auto coor = ui.lineEdit_coor->text();
        for (auto it = mListTest.begin(); it != mListTest.end(); ++it)
        {
            if (it->coordinateL == coor || it->coordinateR == coor)
            {
                ui.tableWidget->selectRow(it->rowNo);
                ui.tableWidget->setFocus();
            }
        }
    });

    connect(ui.btn_find_pin, &QPushButton::clicked, [this]() {
        auto pin = ui.lineEdit_pin->text();
        for (auto it = mListTest.begin(); it != mListTest.end(); ++it)
        {
            if (it->pinL == pin || it->pinR == pin)
            {
                ui.tableWidget->selectRow(it->rowNo);
                ui.tableWidget->setFocus();
            }
        }
    });

    connect(ui.checkBox_short, &QCheckBox::clicked, [this](bool checked) {
        if (checked)
        {
            qDebug() << "Checkbox is now checked.";
            enShort_ = true;
        }
        else
        {
            qDebug() << "Checkbox is now unchecked.";
            enShort_ = false;
        }
    });

    connect(ui.pushButtonFindpoint, &QPushButton::clicked, [this]() {
        QDialog* tpDialog = new QDialog();

        mbExit = false;
        connect(tpDialog, &QDialog::finished, [this](int) {
            mbExit = true;
        });

        mFindPointLabel = new QLabel("pin:    ");
        QFont ft;
        ft.setPointSize(80);
        mFindPointLabel->setFont(ft);
        QVBoxLayout *tpLayout = new QVBoxLayout;

        tpLayout->addWidget(mFindPointLabel);
        tpDialog->setLayout(tpLayout);

        emit signalFindBegin();
        tpDialog->exec();
    });

    connect(ui.pushButtonPause, &QPushButton::clicked, [this]() {
        mbPause = !mbPause;
        mbPause ? ui.pushButtonPause->setText(QStringLiteral("继续")) : ui.pushButtonPause->setText(QStringLiteral("暂停"));
    });

    connect(ui.pushButtonExit, &QPushButton::clicked, [this]() {
        mbExit = true;
        gpSignal->colorSignal(gpUi->pushButtonStart, "QPushButton{background:}");
        ui.labelResult->clear();

        for (auto it = mListTest.begin(); it != mListTest.end(); ++it)
        {
            gpSignal->textSignal(ui.tableWidget->item(it->rowNo, 0), "");
            gpSignal->colorSignal(ui.tableWidget->item(it->rowNo, 0), QColor(255, 255, 255), 0);
        }
    });

    connect(ui.pushButtonStepTest, &QPushButton::clicked, [this]() {
        ui.labelResult->clear();

        if (mListTest.size() < 1)
        {
            QMessageBox::information(this, "", QObject::tr("请先加载测试档案..."));
            gpSignal->colorSignal(gpUi->pushButtonStart, "QPushButton{background:}");
            ui.pushButtonStart->setEnabled(true);
            return;
        }

        auto* widget = ui.tableWidgetNg;
        QList<QTableWidgetItem*> items = widget->selectedItems();
        if (items.count() < 1)
        {
            QMessageBox::information(this, "", QStringLiteral("请选择测试行..."));
            return;
        }

        const int rowSel = widget->row(items.at(0));
        if (rowSel < 0 || rowSel >= widget->rowCount())
            return;

        auto cellText = [widget](int row, int col) -> QString {
            QTableWidgetItem* it = widget->item(row, col);
            return it ? it->text() : QString();
        };

        itemTest tItem;
        tItem.bInlist = true;
        tItem.result = -1;
        tItem.rowNo = rowSel;
        tItem.coordinateL = cellText(rowSel, 1);
        tItem.coordinateR = cellText(rowSel, 2);
        tItem.category = cellText(rowSel, 3);
        tItem.pinL = cellText(rowSel, 8);
        tItem.pinR = cellText(rowSel, 9);

        // Read — must snapshot row data before slotStartList(): it calls tableWidgetNg->clear(),
        // which deletes the selected QTableWidgetItem pointers.
        bStep_ = true;
        slotStartList();

        int rowNg = -1;
        for (int r = 0; r < widget->rowCount(); ++r)
        {
            QTableWidgetItem* il = widget->item(r, 8);
            QTableWidgetItem* ir = widget->item(r, 9);
            if (il && ir && il->text() == tItem.pinL && ir->text() == tItem.pinR)
            {
                rowNg = r;
                break;
            }
        }

        ui.labelCoordinateL->setText(tItem.coordinateL);
        ui.labelCoordinateR->setText(tItem.coordinateR);
        ui.labelPinL->setText(tItem.pinL);
        ui.labelPinR->setText(tItem.pinR);
        ui.labelCategory->setText(tItem.category);

        if (!lineTest(tItem))
        {
            if (rowNg >= 0)
            {
                QTableWidgetItem* status = widget->item(rowNg, 0);
                if (status)
                {
                    gpSignal->textSignal(status, "  NG  ");
                    status->setForeground(QColor(255, 0, 0));
                }
            }
            ui.labelResult->setText(QStringLiteral("<font style='font-size:40px; color:red;'>%0</font>").arg(lineMsg_));

            ng_list_.clear();
            ng_list_.append(tItem);
            saveTestDataFile(true, false);
        }
        else
        {
            if (rowNg >= 0)
            {
                QTableWidgetItem* status = widget->item(rowNg, 0);
                if (status)
                {
                    gpSignal->textSignal(status, "  OK  ");
                    status->setForeground(QColor(0, 255, 0));
                }
            }
            ui.labelResult->setText(QStringLiteral("<font style='font-size:40px; color:green;'>OK: %0 - %1</font>").arg(tItem.pinL).arg(tItem.pinR));

            ng_list_.clear();
            saveTestDataFile(true, true);
        }

        bStep_ = false;
    });

    connect(ui.tableWidget, &QTableWidget::itemSelectionChanged, [this]() {
        auto items = ui.tableWidget->selectedItems();
        auto size = items.count();
        if (size >= 7)
        {
            ui.labelCoordinateL->setText(items[1]->text());
            ui.labelCoordinateR->setText(items[2]->text());
            ui.labelCategory->setText(items[3]->text());
            ui.labelPinL->setText(items[5]->text());
            ui.labelPinR->setText(items[6]->text());
        }
        if (size >= 10)
        {
            ui.labelPinL->setText(items[8]->text());
            ui.labelPinR->setText(items[9]->text());
        }
    });

    connect(this, &QIoTest::signalFind, this, &QIoTest::slotFind);
    connect(this, &QIoTest::signalFindBegin, this, &QIoTest::slotFindBegin, Qt::QueuedConnection);

    connect(ui.pushButtonStart, &QPushButton::clicked, [this]() {
        mbExit = false;
        gpSignal->colorSignal(gpUi->pushButtonStart, "QPushButton{background:lightgreen}");

        for (auto it = mListTest.begin(); it != mListTest.end(); ++it)
        {
            gpSignal->textSignal(ui.tableWidget->item(it->rowNo, 0), " ");
            gpSignal->colorSignal(ui.tableWidget->item(it->rowNo, 0), QColor(255, 255, 255), 0);
        }

        qDebug() << "pushButtonReadSlot...";
        pushButtonReadSlot();
    });

    connect(this, &QIoTest::signalStartList, this, &QIoTest::slotStartList);

    setupConnectReadButtons();
}

QIoTest::~QIoTest()
{
}

void QIoTest::slotFind(QString pin)
{
    mFindPointLabel->setText("Find pin: " + pin);
}

void QIoTest::slotFindBegin()
{
    while (true)
    {
        _sleeploop(100);

        if (mbExit)
        {
            mbExit = false;
            break;
        }

        QByteArray recv;
        if (!gpComClient->communicate(QByteArray::fromHex("AA2800"), recv))
        {
            statusBar()->showMessage(tr("error: AA2800"), 5000);
            continue;
        }

        QByteArray msgBytes = recv;
        const auto isAsciiHex = [](const QByteArray& b) -> bool {
            if (b.isEmpty() || (b.size() % 2) != 0) return false;
            for (const auto ch : b)
            {
                if (!((ch >= '0' && ch <= '9') ||
                      (ch >= 'a' && ch <= 'f') ||
                      (ch >= 'A' && ch <= 'F')))
                {
                    return false;
                }
            }
            return true;
        };
        if (isAsciiHex(recv))
        {
            msgBytes = QByteArray::fromHex(recv);
        }
        std::vector<uint8_t> msg(msgBytes.begin(), msgBytes.end());

        if (msg.size() < 3)
        {
            statusBar()->showMessage(tr("error: msg.size() < 3"), 5000);
            continue;
        }

        if (msg[0] != 0xDE)
        {
            statusBar()->showMessage(tr("error: msg[0]"), 5000);
            continue;
        }

        QStringList v_pins;

        for (size_t i = 1; i < msg.size(); i += 2)
        {
            auto pin = (((uint16_t)msg[i]) << 8) | msg[i + 1];
            uint16_t pin_s = (pin / 64) * 64 + (64 - pin % 64);
            v_pins.append(QString("%0").arg(pin_s));
        }

        emit signalFind(v_pins.join(","));

        // Play sound using QSoundEffect (Qt Multimedia)
        static QSoundEffect soundEffect;
        static bool soundLoaded = false;
        if (!soundLoaded) {
            soundEffect.setSource(QUrl::fromLocalFile(gExePath + "/find.wav"));
            soundLoaded = true;
        }
        soundEffect.play();
    }
}

void QIoTest::pushButtonConnectSlot()
{
    const unsigned port = static_cast<unsigned>(ui.comboBox->currentIndex());
    gpComClient->close();
    gpComClient = std::make_shared<CommunicateClass::ComPortOne>(port);
    if (gpComClient->init()) {
        gpSignal->colorSignal(gpUi->pushButtonConnect, "QPushButton{background:lightgreen}");
	    gpSignal->showDialogSignal(QStringLiteral("<font style='font-size:50px; background-color:white; color:green;'>连接成功</font>"));
        statusBar()->showMessage(QStringLiteral("串口 COM%1 连接成功").arg(port), 3000);
    } else {
        gpSignal->colorSignal(gpUi->pushButtonConnect, "QPushButton{background:red}");
		gpSignal->showDialogSignal(QStringLiteral("<font style='font-size:50px; background-color:white; color:red;'>连接失败</font>"));
        statusBar()->showMessage(QStringLiteral("串口 COM%1 打开失败").arg(port), 5000);
    }
}

void QIoTest::pushButtonReadSlot()
{
    //
	com_pairs_.clear();
	parse_done_ = false;

	if (!msgParse(true))
	{
		gpSignal->colorSignal(gpUi->pushButtonStart, "QPushButton{background:}");
		statusBar()->showMessage(tr("msgParse error: "), 5000);
		gpSignal->showDialogSignal(
			QStringLiteral("<font style='font-size:50px; background-color:white; color:red;'>测试失败</font>"));
		return;
	}

	for (size_t i = 0; i < 100; i++)
	{
		if (parse_done_)
		{
			qDebug() << "signalValuesReady...";
			signalValuesReady();
			break;
		}

		if (!msgParse())
		{
			gpSignal->colorSignal(gpUi->pushButtonStart, "QPushButton{background:}");
			statusBar()->showMessage(tr("msgParse error: "), 5000);
			gpSignal->showDialogSignal(
				QStringLiteral("<font style='font-size:50px; background-color:white; color:red;'>测试失败</font>"));
			return;
		}
	}

}

void QIoTest::setupConnectReadButtons()
{
    connect(ui.pushButtonConnect, &QPushButton::clicked, [this]() {
        pushButtonConnectSlot();
    });

    connect(ui.pushButtonRead, &QPushButton::clicked, [this]() {
        pushButtonReadSlot();
    });

    connect(this, SIGNAL(signalValuesReady()), this, SLOT(slotValuesReady()));
}

void QIoTest::comConnectSources()
{
    // COM port connection setup
}

void QIoTest::updateSets(QVector<QSet<int>>& sets, int L, int R)
{
    // Update sets with L and R values
    for (auto& set : sets)
    {
        if (set.contains(L) || set.contains(R))
        {
            set.insert(L);
            set.insert(R);
            return;
        }
    }
    QSet<int> newSet;
    newSet.insert(L);
    newSet.insert(R);
    sets.append(newSet);
}

void QIoTest::updateTestTask()
{
    testTaskSets.clear();

	for (auto it = mListTest.begin(); it != mListTest.end(); ++it)
	{
		if (!mCurCategorys.contains(it->category))
		{
			it->bInlist = false;
			continue;
		}

		int L = it->pinL.toInt();
		int R = it->pinR.toInt();

		updateSets(testTaskSets, L, R);

	}
}

void QIoTest::slotValuesReady()
{

	updateTestTask();

	//
	if (bStep_)
	{
		return;
	}

	//
	ui.pushButtonStart->setEnabled(false);
	qDebug() << "signalStartList...";
	signalStartList();
}

bool QIoTest::msgParse(bool bfirst)
{
    auto cmd = bfirst ? QByteArray::fromHex("AA2700") : QByteArray::fromHex("AA2701");

	QByteArray recv;
	Dologs::outlog(cmd.toHex());
	if (!gpComClient->communicate(cmd, recv))
	{
		gpSignal->colorSignal(gpUi->pushButtonStart, "QPushButton{background:}");
		statusBar()->showMessage(tr("Write error: "), 5000);
		return false;
	}
	Dologs::outlog(recv.toHex());

	QByteArray msgBytes = recv;
	const auto isAsciiHex = [](const QByteArray& b) -> bool {
		if (b.isEmpty() || (b.size() % 2) != 0) return false;
		for (const auto ch : b)
		{
			if (!((ch >= '0' && ch <= '9') ||
				  (ch >= 'a' && ch <= 'f') ||
				  (ch >= 'A' && ch <= 'F')))
			{
				return false;
			}
		}
		return true;
	};
	if (isAsciiHex(recv))
	{
		msgBytes = QByteArray::fromHex(recv);
	}
	std::vector<uint8_t> msg(msgBytes.begin(), msgBytes.end());

	if (msg.size() < 6)
	{
		statusBar()->showMessage(tr("error: msg.size() < 1"), 5000);
		return false;
	}

	if (msg[0] != 0xDE)
	{
		statusBar()->showMessage(tr("error: msg[0]"), 5000);
		return false;
	}

	if (msg.size() > 256)
	{
		statusBar()->showMessage(tr("error: msg.size() > 255"), 5000);
		return false;
	}

	int index = 1;
	while (index < msg.size())
	{
		int cnt = msg[index];

		if (cnt == 0) // tail fill
		{
			break;
		}

		index++;
		for (int i = 0; i < cnt - 1; i++)
		{
			uint16_t first = (((uint16_t)msg[index + i]) << 8) | msg[index + i + 1];
			uint16_t second = (((uint16_t)msg[index + i + 2]) << 8) | msg[index + i + 3];
			com_pairs_.insert({ first, second });
		}
		index += cnt * 2;
	}

	if (msg.size() < 255)
	{
		parse_done_ = true;
	}

	return true;
}

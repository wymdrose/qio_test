#ifndef QIOTEST_H
#define QIOTEST_H

#include <QMainWindow>
#include "ui_qiotest.h"
#include "global.h"
#include <QList>
#include <set>
#include <QTimer>
#include <QEventLoop>

enum H_L
{
    L, H
};

struct itemTest
{
    bool bInlist;
    int result;
    int rowNo;
    QString coordinateL;
    QString coordinateR;
    QString category;
    QString pinL;
    QString pinR;
};

class QIoTest : public QMainWindow
{
    Q_OBJECT

public:
    explicit QIoTest(QWidget *parent = nullptr);
    ~QIoTest();

    bool lineTest(itemTest item);

public slots:
    void slotStartList();
    void slotFindBegin();
    void slotFind(QString);
    void slotValuesReady();
    
signals:
    void signalStartList();
    void signalFindBegin();
    void signalFind(QString);
    void signalValuesReady();

private:
    void _sleeploop(int ms) {
        QEventLoop loop;
        QTimer::singleShot(ms, &loop, &QEventLoop::quit);
        loop.exec();
    }

    bool msgParse(bool bfirst = false);
    bool saveTestDataFile(bool isStepTest, bool overallOk);

private:
    Ui::QIoTestClass ui;

    QLabel *mFindPointLabel;
    QString mFilePath;
    QString inputFile_;

    void pushButtonConnectSlot();
    void pushButtonReadSlot();
    void setupConnectReadButtons();
    void comConnectSources();
    void updateSets(QVector<QSet<int>>& sets, int L, int R);
    void updatePinLinkGroups();
    void updateTestTask();
    bool checkShort(QSet<int> item, int L, int R);
    bool checkPins(itemTest item);
    bool selfCheck();
    bool shouldSkipTestLine(const QString& category);

    std::vector<std::vector<QString>> num_line_map_; // duizhao
    QMap<QString, QList<QString>> mapTest;  // need to test from category
    std::vector<int> mCurBoards;
    QList<itemTest> mListTest;  // read all items from local file
    QList<itemTest> ng_list_;
    QSet<QString> mCurCategorys;
    bool mbPause = false;
    bool mbExit = false;
    bool bStep_ = false;
    bool enShort_ = false;

    QVector<QSet<int>> pinLinkGroups;
    QVector<QSet<int>> testTaskSets;  // from csv
    std::set<std::set<uint16_t>> com_pairs_;
    bool parse_done_;

    bool findRequest = true;
    int findIndex = 0;

    QSettings settings{ "app.ini", QSettings::IniFormat };
    QString lineMsg_;
};

#endif // QIOTEST_H

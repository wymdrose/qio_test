#ifndef XLSXFILE_H
#define XLSXFILE_H

#include <QTableWidget>
#include <QDateTime>
#include <algorithm>
#include <vector>

#ifdef HAVE_QXLSX
#include <xlsxdocument.h>
#include <xlsxworkbook.h>
#include <xlsxworksheet.h>
#include <xlsxcell.h>
#endif

namespace FileIo
{

class XlsxFile
{
public:
#ifdef HAVE_QXLSX
    void readExcel(const QString path, std::vector<std::vector<QString>>& num_lines)
    {
        QXlsx::Document xlsx(path);
        QXlsx::Workbook *workBook = xlsx.workbook();
        workBook->sheetCount();
        QXlsx::Worksheet *workSheet = static_cast<QXlsx::Worksheet*>(workBook->sheet(0));

        auto b = workSheet->dimension().rowCount();

        auto rows = std::min(b, 1024 * 2);

        num_lines.clear();
        QString value;
        for (int i = 2; i <= rows; i++)
        {
            std::vector<QString> num_line;
            for (int j = 2; j <= 3; j++)
            {
                auto cell = workSheet->cellAt(i, j);
                if (!cell) continue;

                value = cell->value().toString();
                num_line.push_back(value);
            }

            num_lines.push_back(num_line);
        }
    }

    void readExcel(const QString path, const int col, QStringList& input_list)
    {
        QXlsx::Document xlsx(path);
        QXlsx::Workbook *workBook = xlsx.workbook();
        workBook->sheetCount();
        QXlsx::Worksheet *workSheet = static_cast<QXlsx::Worksheet*>(workBook->sheet(0));

        auto b = workSheet->dimension().rowCount();

        auto rows = std::min(b, 1024 * 2);

        input_list.clear();
        QString value;
        for (int i = 1; i <= rows; i++)
        {
            int j = col;

            auto cell = workSheet->cellAt(i, j);
            if (!cell) continue;
            if (cell->isDateTime())
            {
                QDateTime dt = cell->dateTime().toDateTime();
                if (dt.date().year() == 1899) continue;
                value = dt.toString("yyyy/MM/dd hh:mm");
            }
            else
            {
                value = cell->value().toString();
            }

            input_list.append(value);
        }
    }

    void readExcel(QString path, QTableWidget* pTableWidget)
    {
        QXlsx::Document xlsx(path);
        QXlsx::Workbook *workBook = xlsx.workbook();
        workBook->sheetCount();
        QXlsx::Worksheet *workSheet = static_cast<QXlsx::Worksheet*>(workBook->sheet(0));

        auto b = workSheet->dimension().rowCount();
        auto c = workSheet->dimension().columnCount();

        auto rows = std::min(b, 1024 * 2);
        pTableWidget->setRowCount(rows);
        pTableWidget->setColumnCount(c);
        QString value;
        for (int i = 1; i <= rows; i++)
        {
            for (int j = 1; j <= c; j++)
            {
                value.clear();
                auto cell = workSheet->cellAt(i, j);
                if (cell)
                {
                    if (cell->isDateTime())
                    {
                        QDateTime dt = cell->dateTime().toDateTime();
                        if (dt.date().year() != 1899)
                            value = dt.toString("yyyy/MM/dd hh:mm");
                    }
                    else
                    {
                        value = cell->value().toString();
                    }
                }

                QTableWidgetItem *item = new QTableWidgetItem(value);
                pTableWidget->setItem(i - 1, j - 1, item);
            }
        }

        for (int i = rows - 1; i >= 0; i--)
        {
            QTableWidgetItem *item = pTableWidget->item(i, 0);
            if (item == nullptr)
                pTableWidget->removeRow(i);
            else
                break;
        }
    }

    bool writeExcel(QString path, QTableWidget* pTableWidget)
    {
        QXlsx::Document xlsx(path);
        QString value;
        for (int i = 0; i < pTableWidget->rowCount(); i++)
        {
            for (int j = 0; j < pTableWidget->columnCount(); j++)
            {
                QTableWidgetItem *item = pTableWidget->item(i, j);
                if (item == nullptr)
                    value = "";
                else
                    value = item->text();
                xlsx.write(i + 1, j + 1, value);
            }
        }

        if (!xlsx.save())
            return false;

        return true;
    }

#else
    // Stub implementations when QXlsx is not available
    void readExcel(const QString, std::vector<std::vector<QString>>&)
    {
        qWarning() << "QXlsx not available";
    }

    void readExcel(const QString, const int, QStringList&)
    {
        qWarning() << "QXlsx not available";
    }

    void readExcel(QString, QTableWidget*)
    {
        qWarning() << "QXlsx not available";
    }

    bool writeExcel(QString, QTableWidget*)
    {
        qWarning() << "QXlsx not available";
        return false;
    }
#endif

private:
    QTableWidget* mpTableWidget;
};

}  // namespace FileIo

#endif // XLSXFILE_H

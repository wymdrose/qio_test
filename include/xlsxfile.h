#ifndef XLSXFILE_H
#define XLSXFILE_H

#include <QTableWidget>
#include <QDateTime>
#include <algorithm>
#include <stdexcept>
#include <vector>

#ifdef HAVE_XLNT
#include <xlnt/xlnt.hpp>
#include "xlsx_load.h"
#endif

namespace FileIo
{

#ifdef HAVE_XLNT
namespace {

[[noreturn]] void throwLoadError(const QString &error)
{
    throw std::runtime_error(error.toUtf8().toStdString());
}

QString cellToQString(const xlnt::cell &cell)
{
    if (!cell.has_value())
        return QString();

    if (cell.is_date())
    {
        try
        {
            const xlnt::datetime dt = cell.value<xlnt::datetime>();
            if (dt.is_null() || dt.get_year() == 1899)
                return QString();

            const QDateTime qdt(
                QDate(dt.get_year(), dt.get_month(), dt.get_day()),
                QTime(dt.get_hour(), dt.get_minute(), dt.get_second()));
            return qdt.toString(QStringLiteral("yyyy/MM/dd hh:mm"));
        }
        catch (...)
        {
            return QString::fromStdString(cell.to_string());
        }
    }

    return QString::fromStdString(cell.to_string());
}

xlnt::worksheet firstSheet(xlnt::workbook &workbook)
{
    if (workbook.sheet_count() == 0)
        return workbook.create_sheet();

    return workbook.sheet_by_index(0);
}

const xlnt::worksheet firstSheet(const xlnt::workbook &workbook)
{
    return workbook.sheet_by_index(0);
}

int sheetRowCount(const xlnt::worksheet &worksheet)
{
    const auto dim = worksheet.calculate_dimension();
    return static_cast<int>(dim.bottom_right().row());
}

int sheetColumnCount(const xlnt::worksheet &worksheet)
{
    const auto dim = worksheet.calculate_dimension();
    return static_cast<int>(dim.bottom_right().column_index());
}

QString cellText(const xlnt::worksheet &worksheet, int row, int column)
{
    const xlnt::column_t col(column);
    const xlnt::row_t r(row);
    const xlnt::cell_reference ref{col, r};
    if (!worksheet.has_cell(ref))
        return QString();

    return cellToQString(worksheet.cell(col, r));
}

} // namespace
#endif

class XlsxFile
{
public:
#ifdef HAVE_XLNT
    void readExcel(const QString path, std::vector<std::vector<QString>>& num_lines)
    {
        xlnt::workbook workbook;
        QString error;
        if (!loadWorkbook(workbook, path, &error))
            throwLoadError(error);

        const xlnt::worksheet worksheet = firstSheet(workbook);

        const int rows = std::min(sheetRowCount(worksheet), 1024 * 2);

        num_lines.clear();
        for (int i = 2; i <= rows; i++)
        {
            std::vector<QString> num_line;
            for (int j = 2; j <= 3; j++)
            {
                const QString value = cellText(worksheet, i, j);
                if (value.isEmpty())
                    continue;

                num_line.push_back(value);
            }

            if (num_line.size() >= 2)
                num_lines.push_back(num_line);
        }
    }

    void readExcel(const QString path, const int col, QStringList& input_list)
    {
        xlnt::workbook workbook;
        QString error;
        if (!loadWorkbook(workbook, path, &error))
            throwLoadError(error);

        const xlnt::worksheet worksheet = firstSheet(workbook);

        const int rows = std::min(sheetRowCount(worksheet), 1024 * 2);

        input_list.clear();
        for (int i = 1; i <= rows; i++)
        {
            const QString value = cellText(worksheet, i, col);
            if (!value.isEmpty())
                input_list.append(value);
        }
    }

    void readExcel(QString path, QTableWidget* pTableWidget)
    {
        xlnt::workbook workbook;
        QString error;
        if (!loadWorkbook(workbook, path, &error))
            throwLoadError(error);

        const xlnt::worksheet worksheet = firstSheet(workbook);

        const int rows = std::min(sheetRowCount(worksheet), 1024 * 2);
        const int cols = sheetColumnCount(worksheet);

        pTableWidget->setRowCount(rows);
        pTableWidget->setColumnCount(cols);
        for (int i = 1; i <= rows; i++)
        {
            for (int j = 1; j <= cols; j++)
            {
                const QString value = cellText(worksheet, i, j);
                auto *item = new QTableWidgetItem(value);
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
        xlnt::workbook workbook;
        QString error;
        if (!loadWorkbook(workbook, path, &error))
            return false;

        xlnt::worksheet worksheet = firstSheet(workbook);

        for (int i = 0; i < pTableWidget->rowCount(); i++)
        {
            for (int j = 0; j < pTableWidget->columnCount(); j++)
            {
                QTableWidgetItem *item = pTableWidget->item(i, j);
                const QString value = item ? item->text() : QString();
                worksheet.cell(xlnt::column_t(j + 1), xlnt::row_t(i + 1)).value(value.toStdString());
            }
        }

        workbook.save(path.toStdWString());
        return true;
    }

#else
    void readExcel(const QString, std::vector<std::vector<QString>>&)
    {
        qWarning() << "xlnt not available";
    }

    void readExcel(const QString, const int, QStringList&)
    {
        qWarning() << "xlnt not available";
    }

    void readExcel(QString, QTableWidget*)
    {
        qWarning() << "xlnt not available";
    }

    bool writeExcel(QString, QTableWidget*)
    {
        qWarning() << "xlnt not available";
        return false;
    }
#endif

private:
    QTableWidget* mpTableWidget;
};

}  // namespace FileIo

#endif // XLSXFILE_H

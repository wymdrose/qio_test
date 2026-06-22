#ifndef XLSX_LOAD_H
#define XLSX_LOAD_H

#include <QString>
#include <xlnt/xlnt.hpp>

namespace FileIo
{

bool loadWorkbook(xlnt::workbook &workbook, const QString &path, QString *error = nullptr);

} // namespace FileIo

#endif

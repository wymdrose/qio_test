#include "xlsx_load.h"

#include <iostream>
#include <xlnt/xlnt.hpp>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: load_workbook_test <path>\n";
        return 1;
    }

    xlnt::workbook workbook;
    QString error;
    const bool ok = FileIo::loadWorkbook(workbook, QString::fromUtf8(argv[1]), &error);
    if (ok)
    {
        std::cout << "loadWorkbook OK sheets=" << workbook.sheet_count() << std::endl;
        return 0;
    }

    std::cerr << "loadWorkbook FAILED: " << error.toStdString() << std::endl;
    return 2;
}

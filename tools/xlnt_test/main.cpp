#include <iostream>
#include <string>
#include <xlnt/xlnt.hpp>
#if defined(_WIN32)
#include <windows.h>
#endif
int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "usage: xlnt_test <path>\n"; return 1; }
    try {
        xlnt::workbook wb;
#if defined(_WIN32)
        int wlen = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, nullptr, 0);
        std::wstring wpath(static_cast<std::size_t>(wlen), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, wpath.data(), wlen);
        wb.load(wpath);
#else
        wb.load(argv[1]);
#endif
        std::cout << "OK sheets=" << wb.sheet_count() << std::endl;
        for (std::size_t i = 0; i < wb.sheet_count(); ++i) {
            const auto ws = wb.sheet_by_index(i);
            std::cout << "  sheet " << i << ": " << ws.title() << std::endl;
            auto dim = ws.calculate_dimension();
            std::cout << "    dim " << dim.to_string() << std::endl;
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 2;
    }
}

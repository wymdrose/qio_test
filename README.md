# QIoTest - IO Testing Application

A Qt-based IO testing application for conductivity detection, featuring serial port communication and Excel file support.

## Features

- Serial port communication for hardware testing
- Excel file reading/writing (via xlnt library)
- SQLite database for data storage
- Test result tracking with OK/NG status
- Coordinate and pin point searching
- Short circuit detection

## Requirements

- Qt 5.15 (or compatible 5.x), e.g. via vcpkg (`qt5-base`, `qt5-serialport`, `qt5-multimedia`)
- CMake 3.16 or later
- C++17 compatible compiler
- xlnt (via vcpkg)

### Required Qt5 modules

- Qt5::Core
- Qt5::Gui
- Qt5::Widgets
- Qt5::SerialPort
- Qt5::Sql
- Qt5::Multimedia

## Building

### Windows (with Qt Creator)

1. Open `CMakeLists.txt` with Qt Creator
2. Configure with a Qt 5 kit (or a CMake toolchain that supplies Qt5, such as vcpkg)
3. Build and run

### Command Line Build

```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" ..
cmake --build . --config Release
```

### Windows installer (Release EXE)

Build a self-contained NSIS installer that bundles Qt runtime, config files, and the app:

1. Install [NSIS](https://nsis.sourceforge.io/) (once): `winget install NSIS.NSIS`
2. From the project root:

```bat
configure.bat
build_installer.bat
```

Output: `build\QIoTest-<version>-win64.exe`

The installer creates Start Menu and desktop shortcuts and registers an uninstaller in Windows Settings.

To deploy Qt DLLs next to the exe without building an installer (for local testing):

```bat
build_release.bat
scripts\deploy_release.cmd
```

## Project Structure

```
io_test/
├── CMakeLists.txt          # Main CMake configuration
├── README.md               # This file
├── src/                    # Source files
│   ├── main.cpp           # Application entry point
│   ├── qiotest.h          # Main window header
│   ├── qiotest.cpp        # Main window implementation
│   ├── testprocess.cpp    # Test processing logic
│   └── global.h           # Global declarations
├── include/               # Header-only libraries
│   ├── communicatelib.h   # Serial communication
│   ├── xlsxfile.h         # Excel file handling
│   ├── mysignal.h         # Qt signal helpers
│   ├── mysqlite.h         # SQLite wrapper
│   └── dologs.h           # Logging utilities
├── ui/                    # UI files
│   └── qiotest.ui         # Main window UI
├── resources/             # Resources
│   ├── qiotest.qrc        # Resource file
│   └── Arrow.png          # Arrow icon
└── cfg/                   # Configuration files
    └── duizhao.xlsx       # Mapping table (place here)
```

## Configuration

The application uses an `app.ini` file to store settings:

- `com_port`: Selected COM port index
- `file_path`: Last opened test file path
- `input_path`: Last opened input file path

## Excel support (xlnt)

Excel read/write is provided by [xlnt](https://github.com/xlnt-community/xlnt), installed through vcpkg (`xlnt` in `vcpkg.json`). Install into the global vcpkg tree (`D:\vcpkg\installed`) with:

```bat
D:\vcpkg\vcpkg.exe install xlnt:x64-windows --classic
```

Then re-run `configure.bat`.

## License

Proprietary - Dongguan Jingwei Intelligent Technology

## Version

v3.0.12

v4.0.x
cmake --build build --config Debug --clean-first
cmake --build build --config Release --clean-first
.\build_release.bat --clean-first
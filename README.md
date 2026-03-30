# QIoTest - IO Testing Application

A Qt6-based IO testing application for conductivity detection, featuring serial port communication, Modbus TCP, and Excel file support.

## Features

- Serial port communication for hardware testing
- Modbus TCP client support
- Excel file reading/writing (requires QXlsx library)
- SQLite database for data storage
- Test result tracking with OK/NG status
- Coordinate and pin point searching
- Short circuit detection

## Requirements

- Qt 6.2 or later
- CMake 3.16 or later
- C++17 compatible compiler
- Optional: QXlsx library for Excel file support

### Required Qt6 Modules

- Qt6::Core
- Qt6::Gui
- Qt6::Widgets
- Qt6::Network
- Qt6::SerialPort
- Qt6::SerialBus
- Qt6::Sql
- Qt6::Multimedia

## Building

### Windows (with Qt Creator)

1. Open `CMakeLists.txt` with Qt Creator
2. Configure the project with your Qt6 kit
3. Build and run

### Command Line Build

```bash
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64" ..
cmake --build . --config Release
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
│   ├── communicatelib.h   # TCP/Serial communication
│   ├── xlsxfile.h         # Excel file handling
│   ├── mysignal.h         # Qt signal helpers
│   ├── mysqlite.h         # SQLite wrapper
│   ├── dologs.h           # Logging utilities
│   └── modbusmodel.h      # Modbus helper
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

## Optional: Installing QXlsx

To enable Excel file support, install QXlsx:

```bash
# Clone QXlsx
git clone https://github.com/QtExcel/QXlsx.git

# Build and install
cd QXlsx
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64" ..
cmake --build . --config Release
cmake --install . --prefix "C:/QXlsx"
```

Then add to your CMake configure command:
```bash
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64;C:/QXlsx" ..
```

## License

Proprietary - Dongguan Jingwei Intelligent Technology

## Version

v3.0.12

v4.0.0
cmake --build build --config Debug --clean-first
cmake --build build --config Release --clean-first

# PDS-T1000-TSA24 - AI SYSTEM GUIDE
**Complete Live Car Detection System with ESP32 ESP-NOW Bridge**

## SYSTEM OVERVIEW
```
Camera → Python Detection → C++ App → UART (COM5) → ESP32 → ESP-NOW → Target Device
```

Live car detection system that:
- Detects colored objects via camera (Python OpenCV)
- Displays real-time visualization (C++ Raylib)
- Sends coordinates via UART to ESP32
- ESP32 forwards via ESP-NOW to target MAC `74:4D:BD:A1:BF:04`

## QUICK START COMMANDS

### 1. Build System
```powershell
cd "c:\Users\jonas\OneDrive\Documents\GitHub\PDS-T1000-TSA24"
g++ -std=c++17 -Wall -Wextra -Iexternal/raylib/src -Iinclude -IC:/Program Files/Python311/Include src/main.cpp src/py_runner.cpp src/car_simulation.cpp src/uart_communication.cpp -Lexternal/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -LC:/Program Files/Python311/libs -lpython311 -o main_uart.exe
```

### 2. Flash ESP32
```powershell
cd "c:\Users\jonas\OneDrive\Documents\GitHub\PDS-T1000-TSA24\esp_vehicle"
$env:IDF_PATH="C:\Users\jonas\esp\v5.4.2\esp-idf"
$env:PATH="$env:PATH;C:\Espressif\tools\ninja\1.12.1;C:\Espressif\tools\xtensa-esp-elf\esp-14.2.0_20241119\xtensa-esp-elf\bin"
C:\Espressif\python_env\idf5.4_py3.11_env\Scripts\python.exe C:\Users\jonas\esp\v5.4.2\esp-idf\tools\idf.py build
C:\Espressif\python_env\idf5.4_py3.11_env\Scripts\python.exe C:\Users\jonas\esp\v5.4.2\esp-idf\tools\idf.py -p COM5 flash
```

### 3. Run System
```powershell
.\main_uart.exe
```

## REBUILD AFTER CHANGES

### Main System (after C++ code changes):
```powershell
g++ -std=c++17 -Wall -Wextra -Iexternal/raylib/src -Iinclude -IC:/Program Files/Python311/Include src/main.cpp src/py_runner.cpp src/car_simulation.cpp src/uart_communication.cpp -Lexternal/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -LC:/Program Files/Python311/libs -lpython311 -o main_uart.exe
```

### ESP32 (after ESP32 code changes):
```powershell
cd esp_vehicle
$env:IDF_PATH="C:\Users\jonas\esp\v5.4.2\esp-idf"
$env:PATH="$env:PATH;C:\Espressif\tools\ninja\1.12.1;C:\Espressif\tools\xtensa-esp-elf\esp-14.2.0_20241119\xtensa-esp-elf\bin"
C:\Espressif\python_env\idf5.4_py3.11_env\Scripts\python.exe C:\Users\jonas\esp\v5.4.2\esp-idf\tools\idf.py build
C:\Espressif\python_env\idf5.4_py3.11_env\Scripts\python.exe C:\Users\jonas\esp\v5.4.2\esp-idf\tools\idf.py -p COM5 flash
cd ..
```

### Test Tool (uart_testen.cpp):
```powershell
g++ -std=c++17 -Wall -Wextra src/uart_testen.cpp -o uart_test.exe
```

**Note**: Python code (Farberkennung.py) needs no rebuild - just restart main_uart.exe

## FILE STRUCTURE
```
PDS-T1000-TSA24/
├── src/
│   ├── main.cpp                    # Main application with UART + camera
│   ├── py_runner.cpp              # Python integration for camera detection
│   ├── car_simulation.cpp         # Raylib visualization
│   ├── uart_communication.cpp     # ESP32 UART communication
│   ├── Farberkennung.py          # Python camera detection script
│   └── __pycache__/
├── include/
│   ├── py_runner.h
│   ├── car_simulation.h
│   ├── uart_communication.h
│   └── Vehicle.h
├── esp_vehicle/                   # ESP32 ESP-IDF project
│   ├── main/
│   │   └── main.cpp              # ESP32 UART-to-ESP-NOW bridge
│   ├── CMakeLists.txt
│   ├── sdkconfig
│   └── build/
├── external/
│   └── raylib/                   # Raylib graphics library
├── esp32_uart_bridge.ino         # Arduino version (not used)
├── main_uart.exe                 # Main executable
└── AI_SYSTEM_GUIDE.md            # This documentation
```

## TECHNICAL DETAILS

### Dependencies
- **C++ Compiler**: MinGW-w64 g++ with C++17
- **Python**: 3.11 with OpenCV 4.12.0
- **Graphics**: Raylib 5.6-dev
- **ESP32**: ESP-IDF v5.4.2, Ninja build system
- **Hardware**: ESP32 on COM5 @ 115200 baud

### Communication Protocol
- **Format**: `COORD:ObjectColor:x,y\n`
- **Example**: `COORD:Heck1:272,158\n`
- **Frequency**: Every 100ms for live updates
- **Target MAC**: `74:4D:BD:A1:BF:04` (ESP-NOW)

### System Configuration
- **Field Size**: 182x93 grid
- **Camera**: OpenCV backend 700 (DirectShow)
- **UART**: COM5, 115200 baud, 8N1
- **ESP32**: Channel 1 ESP-NOW, primary interface

## TROUBLESHOOTING

### Build Issues
- Ensure Python 3.11 is in `C:/Program Files/Python311/`
- Verify Raylib is built in `external/raylib/src/`
- Check MinGW-w64 g++ is in PATH

### ESP32 Flash Issues
- Check COM port: `Get-WmiObject -Class Win32_SerialPort`
- Verify ESP-IDF environment variables are set
- Ensure all toolchain components in PATH

### Runtime Issues
- Camera detection requires proper lighting
- UART connection shows status in terminal
- ESP32 must be powered and connected to COM5

## ESP32 CODE SUMMARY
The ESP32 firmware (`esp_vehicle/main/main.cpp`) creates a UART-to-ESP-NOW bridge:
1. Receives coordinates via UART from PC
2. Parses format `COORD:Object:x,y`
3. Forwards via ESP-NOW to target device
4. Handles WiFi/ESP-NOW initialization automatically

## AI ASSISTANCE NOTES
- Use exact commands above for building/flashing
- ESP32 is always on COM5 (Silicon Labs CP210x)
- System sends ALL detected coordinates live (not just Heck2)
- No Arduino IDE needed - everything via ESP-IDF terminal
- Main executable is `main_uart.exe` (not `main.exe`)
- After any C++ code changes: rebuild main_uart.exe
- After ESP32 code changes: rebuild and flash ESP32
- Python changes need no rebuild - just restart application
- Current coordinate format: `COORD:ObjectColor:x,y\n`
- ESP32 target MAC address: `74:4D:BD:A1:BF:04`
- UART: COM5 @ 115200 baud, transmission every 100ms

# Build-Konfiguration PDS-T1000-TSA24

## ✅ Funktionierende Executable: main_uart.exe

**WICHTIG**: Verwende ausschließlich `main_uart.exe` - diese Datei ist vollständig kompiliert und funktional.

### Schnellstart:
```bash
# Option 1: Direkt
.\main_uart.exe

# Option 2: Mit Batch-Datei
.\START_SYSTEM.bat
```

### Build-Details:
- **Compiler**: MinGW-w64 15.1.0
- **Raylib**: 5.6-dev 
- **Python**: 3.13.5 Integration
- **UART**: Windows Serial API
- **OpenCV**: 4.12.0

### Verfügbare Tasks (VS Code):
```json
{
  "Build Main": "Erstellt main.exe (Probleme mit UART-Dependencies)",
  "Run Main": "Führt main.exe aus (funktioniert nicht)",
  "Run Farberkennung": "Python-Standalone Test",
  "ESP32 Tasks": "ESP-IDF Build/Flash/Monitor"
}
```

### ⚠️ Nicht verwenden:
- `main.exe` (Build-Probleme mit UARTCommunication)
- `Build Main` Task (Dependency-Fehler)
- `Run Main` Task (ausführt falsche .exe)

### ESP32 Setup (einmalig):
```powershell
cd esp_vehicle
C:\Espressif\python_env\idf5.4_py3.11_env\Scripts\python.exe C:\Users\jonas\esp\v5.4.2\esp-idf\tools\idf.py -p COM5 flash
```

### Systemanforderungen:
- Windows 10/11
- ESP32 auf COM5
- Kamera 1 verfügbar
- Python 3.11+ mit OpenCV

---
**Status**: PRODUKTIONSBEREIT ✅
**Letzte Validierung**: 5. August 2025
**Empfohlene Verwendung**: `.\main_uart.exe`

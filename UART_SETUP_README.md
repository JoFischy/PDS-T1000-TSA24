# ESP32 UART Kommunikation Setup

Dieses Setup ermöglicht die serielle Kommunikation zwischen einem Windows-PC und einem ESP32 für ESP-NOW Fahrzeug-Kommunikation.

## Überblick

- **Windows PC**: Sendet Koordinatendaten über UART an ESP32
- **ESP32**: Empfängt UART-Daten und sendet sie über ESP-NOW an andere Fahrzeuge

## Dateien

### ESP32 (Empfänger)
- `esp_vehicle/main/main.cpp` - Hauptprogramm für ESP32
- Empfängt serielle Daten im Format: `X:12.34;Y:56.78;`
- Sendet Koordinaten über ESP-NOW an andere Fahrzeuge

### Windows PC (Sender)
- `src/uart_sender.cpp` - Windows-Programm zum Senden von Koordinaten
- `build_uart_sender.bat` - Build-Skript für Windows
- `Makefile.uart` - Makefile für manuelle Kompilierung

## Setup und Verwendung

### 1. ESP32 Setup

1. ESP-IDF installieren und konfigurieren
2. Im Verzeichnis `esp_vehicle/` ausführen:
   ```bash
   idf.py build
   idf.py -p COM3 flash
   idf.py -p COM3 monitor
   ```

### 2. Windows Sender Setup

#### Option A: Mit Batch-Datei (empfohlen)
```cmd
# Einfach ausführen:
build_uart_sender.bat
```

#### Option B: Manuell mit MinGW
```cmd
g++ -std=c++17 -Wall -Wextra -O2 src/uart_sender.cpp -o uart_sender.exe -static-libgcc -static-libstdc++
```

#### Option C: Mit Visual Studio
```cmd
cl /EHsc /std:c++17 src/uart_sender.cpp /Fe:uart_sender.exe
```

### 3. Ausführung

1. ESP32 mit USB verbinden und flashen
2. COM-Port des ESP32 notieren (z.B. COM4)
3. Im Windows-Programm `uart_sender.cpp` den richtigen COM-Port einstellen
4. Windows-Programm ausführen:
   ```cmd
   uart_sender.exe
   ```

## Datenformat

### UART Protokoll
```
Format: X:<x-wert>;Y:<y-wert>;
Beispiel: X:12.34;Y:56.78;
```

### ESP-NOW Datenstruktur
```cpp
typedef struct {
    float x;          // X-Koordinate
    float y;          // Y-Koordinate  
    uint32_t timestamp; // Zeitstempel in ms
} coordinate_data_t;
```

## Konfiguration

### ESP32 Konfiguration
- **UART Port**: UART_NUM_0 (Standard USB-Serial)
- **Baudrate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None

### Windows Konfiguration
- **COM Port**: COM4 (anpassbar in uart_sender.cpp)
- **Baudrate**: 115200
- **Sendeintervall**: 1 Sekunde

## Fehlerbehebung

### Häufige Probleme

1. **"COM-Port kann nicht geöffnet werden"**
   - Prüfen Sie den korrekten COM-Port im Windows Geräte-Manager
   - Stellen Sie sicher, dass keine andere Anwendung den Port verwendet
   - ESP32 neu verbinden

2. **"g++ nicht gefunden"**
   - MinGW installieren: https://www.mingw-w64.org/
   - Oder Visual Studio Build Tools verwenden

3. **ESP32 empfängt keine Daten**
   - Baudrate prüfen (beide Seiten 115200)
   - UART-Pins prüfen (RX/TX nicht vertauscht)
   - ESP32 Monitor verwenden: `idf.py monitor`

4. **ESP-NOW funktioniert nicht**
   - MAC-Adressen in main.cpp anpassen
   - WiFi-Modus prüfen (WIFI_MODE_STA)
   - Beide ESP32s müssen ESP-NOW initialisiert haben

### Debugging

#### ESP32 Logs anzeigen:
```bash
idf.py -p COM3 monitor
```

#### Windows Debug-Ausgabe:
Das Programm zeigt alle gesendeten Daten und Fehler in der Konsole an.

## Erweiterungen

### Mögliche Verbesserungen:
1. **Acknowledgment**: ESP32 bestätigt empfangene Daten
2. **Fehlerkorrektur**: Prüfsummen für Datenintegrität
3. **Mehrere Fahrzeuge**: Unterstützung für mehr als 2 Fahrzeuge
4. **Dateieingabe**: Koordinaten aus Datei laden statt Zufallswerte
5. **GUI**: Windows-Interface für einfachere Bedienung

### Beispiel für Dateieingabe:
```cpp
// In uart_sender.cpp ersetzen:
std::ifstream file("coordinates.txt");
std::string line;
while (std::getline(file, line)) {
    // Parse x,y from line and send
}
```

## Hardware-Verbindung

```
Windows PC <--USB--> ESP32 #1 <--ESP-NOW--> ESP32 #2
```

- USB-Verbindung für serielle Kommunikation
- ESP-NOW für drahtlose Fahrzeug-zu-Fahrzeug Kommunikation
- Beide ESP32s müssen auf demselben WiFi-Kanal sein

## Status LEDs (optional)

Für besseres Debugging können Status-LEDs angeschlossen werden:
- GPIO 2: UART Daten empfangen
- GPIO 4: ESP-NOW Daten gesendet
- GPIO 5: ESP-NOW Daten empfangen

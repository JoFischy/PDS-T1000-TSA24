# ESP32 Setup und Flash-Anweisungen

## 🚀 Schnellstart

### Automatisches Setup
```powershell
# Führen Sie das automatische Setup-Skript aus:
.\SETUP_ESP32_LIVE.bat

# Oder für manuelles Monitoring:
.\F5_Monitor2.bat
```

## 📋 Voraussetzungen

### Hardware
- ESP32 DevKit v1 oder kompatibel
- USB-A zu Micro-USB Kabel
- Windows PC mit verfügbarem USB-Port

### Software
- **ESP-IDF v4.4+** (Espressif IoT Development Framework)
- **Python 3.8-3.11** (für ESP-IDF Tools)
- **Git** (für Repository-Management)
- **Drivers** für ESP32 (CP210x oder CH340)

## 🔧 ESP-IDF Installation

### 1. ESP-IDF Installer herunterladen
```powershell
# Von der offiziellen Espressif-Website:
# https://dl.espressif.com/dl/esp-idf/

# Oder mit Scoop (falls installiert):
scoop install esp-idf
```

### 2. Installation und Setup
```powershell
# ESP-IDF Tools installieren
./install.ps1

# Umgebung einrichten (in jeder neuen PowerShell-Session ausführen)
./export.ps1

# Oder dauerhaft zur PATH hinzufügen:
$env:PATH += ";C:\esp\esp-idf\tools"
```

## 📱 ESP32 Hardware-Setup

### Pin-Konfiguration
```
ESP32 DevKit v1 Pinout:
├── GPIO 18 → Ultraschall Trigger
├── GPIO 19 → Ultraschall Echo  
├── GPIO 21 → Motor Links PWM
├── GPIO 22 → Motor Rechts PWM
├── GPIO 25 → Servo (Lenkung)
├── GPIO 26 → LED Status
├── GPIO 0  → Boot Button
└── EN      → Reset Button
```

### Verbindungen prüfen
```powershell
# COM-Ports anzeigen
Get-WmiObject -Class Win32_SerialPort | Select-Object Name, DeviceID

# Oder mit Device Manager
devmgmt.msc
```

## 🔨 Projekt kompilieren

### 1. Projekt-Verzeichnis wechseln
```powershell
cd esp_vehicle
```

### 2. Projektabhängigkeiten installieren
```powershell
# ESP-IDF Komponenten aktualisieren
idf.py reconfigure

# Menükonfiguration (optional)
idf.py menuconfig
```

### 3. Kompilierung
```powershell
# Projekt kompilieren
idf.py build

# Bei Problemen: Clean Build
idf.py fullclean
idf.py build
```

## ⚡ ESP32 Flash-Prozess

### Automatisches Flashing
```powershell
# ESP32 in Download-Modus versetzen und flashen
idf.py flash

# Mit Monitor
idf.py flash monitor

# Spezifischen Port angeben
idf.py -p COM3 flash monitor
```

### Manueller Flash-Prozess

#### 1. ESP32 in Download-Modus versetzen
1. **BOOT-Taste** gedrückt halten
2. **EN-Taste** kurz drücken (Reset)
3. **BOOT-Taste** loslassen
4. ESP32 ist jetzt im Download-Modus

#### 2. Flash-Befehl ausführen
```powershell
# Vollständiger Flash mit allen Partitionen
idf.py -p COM3 erase_flash
idf.py -p COM3 flash

# Oder nur die App-Partition
idf.py -p COM3 app-flash
```

#### 3. Monitoring starten
```powershell
# Monitor für Debug-Ausgaben
idf.py -p COM3 monitor

# Monitor beenden: Ctrl+]
```

## 🔍 Troubleshooting

### Häufige Probleme

#### ESP32 nicht erkannt
```
Problem: ESP32 wird nicht als COM-Port erkannt
Lösung:
1. USB-Kabel prüfen (Daten + Strom)
2. Treiber installieren (CP210x/CH340)
3. Anderen USB-Port versuchen
4. Device Manager prüfen
```

#### Flash-Fehler
```
Problem: "Failed to connect to ESP32"
Lösung:
1. ESP32 in Download-Modus versetzen
2. COM-Port korrekt angeben
3. Andere Programme schließen (Arduino IDE, etc.)
4. ESP32 Reset durchführen
```

#### Kompilierungsfehler
```
Problem: Build-Fehler oder fehlende Abhängigkeiten
Lösung:
1. ESP-IDF Version prüfen (idf.py --version)
2. Projekt neu konfigurieren (idf.py reconfigure)
3. Clean Build durchführen
4. Komponentenverzeichnis prüfen
```

#### Monitor-Probleme
```
Problem: Keine Ausgabe im Monitor
Lösung:
1. Baud-Rate prüfen (115200)
2. Richtigen COM-Port verwenden
3. ESP32 Reset nach Flash
4. Monitor neu starten
```

## 📊 Debug und Monitoring

### Serial Monitor
```powershell
# Standard-Monitor
idf.py monitor

# Mit Filter für bestimmte Tags
idf.py monitor --print_filter "*:I uart:D"

# Baud-Rate ändern
idf.py monitor -b 921600
```

### Debug-Ausgaben im Code
```c
#include "esp_log.h"

static const char* TAG = "MAIN";

void app_main() {
    ESP_LOGI(TAG, "System gestartet");
    ESP_LOGD(TAG, "Debug-Information: %d", value);
    ESP_LOGW(TAG, "Warnung: Sensor nicht verfügbar");
    ESP_LOGE(TAG, "Fehler: Verbindung verloren");
}
```

### Log-Level einstellen
```c
// In menuconfig oder code:
esp_log_level_set("*", ESP_LOG_INFO);
esp_log_level_set("uart", ESP_LOG_DEBUG);
esp_log_level_set("wifi", ESP_LOG_WARN);
```

## ⚙️ Konfiguration

### WiFi-Setup (optional)
```c
// In main/main.c
#define WIFI_SSID "ESP32_Vehicle"
#define WIFI_PASS "vehicle123"

void wifi_init() {
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}
```

### UART-Konfiguration
```c
// UART-Parameter
#define UART_NUM UART_NUM_0
#define BAUD_RATE 115200
#define TX_PIN 1
#define RX_PIN 3

void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, 1024, 1024, 0, NULL, 0);
}
```

## 🎯 Performance-Optimierung

### Task-Prioritäten
```c
// FreeRTOS Tasks
xTaskCreate(uart_task, "uart_task", 2048, NULL, 10, NULL);
xTaskCreate(sensor_task, "sensor_task", 2048, NULL, 5, NULL);
xTaskCreate(motor_task, "motor_task", 2048, NULL, 8, NULL);
```

### Speicher-Monitoring
```c
void print_memory_info() {
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Min free heap: %d bytes", esp_get_minimum_free_heap_size());
}
```

## 📋 Checkliste für Live-Betrieb

### Vor dem Flash
- [ ] Hardware-Verbindungen prüfen
- [ ] COM-Port identifiziert
- [ ] ESP-IDF Umgebung geladen
- [ ] Projekt erfolgreich kompiliert

### Nach dem Flash
- [ ] ESP32 startet erfolgreich
- [ ] UART-Kommunikation funktioniert
- [ ] Sensoren liefern Daten
- [ ] Motor-Steuerung reagiert
- [ ] Status-LED funktioniert

### Für Dauerbetrieb
- [ ] Stromversorgung stabil
- [ ] Überhitzungsschutz
- [ ] Fehlerbehandlung implementiert
- [ ] Watchdog konfiguriert

## 🔗 Nützliche Befehle

```powershell
# ESP32 Informationen abrufen
idf.py read_flash 0x1000 0x100 chip_info.bin

# Partition-Tabelle anzeigen
idf.py partition_table

# Flash-Größe prüfen
esptool.py --port COM3 flash_id

# Bootloader neu flashen
idf.py bootloader-flash

# NVS-Partition löschen
idf.py erase_flash
```

---

**Autor**: ESP32 Team TSA24  
**Version**: 1.2.0  
**Letztes Update**: August 2025

**Support**: Bei Problemen Issues auf GitHub erstellen oder Debug-Logs bereitstellen.

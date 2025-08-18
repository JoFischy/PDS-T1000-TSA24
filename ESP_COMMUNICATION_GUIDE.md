# ESP Serielle Kommunikation - Anleitung

## Überblick

Die PC-Anwendung kann jetzt Fahrzeugbefehle aus der `vehicle_commands.json` seriell an das ESP-Board senden, welches diese dann per ESP-NOW an die entsprechenden Empfängerautos weiterleitet.

## Funktionsweise

### 1. PC-Anwendung (main.exe)
- Liest die erkannten Fahrzeuge aus der Kamera
- Berechnet die benötigten Bewegungsrichtungen basierend auf den Routen
- Schreibt Befehle in `vehicle_commands.json` (Format: `{"id": X, "command": Y}`)
- Sendet diese Befehle seriell an das ESP-Board im Format: `"direction,speed\n"`

### 2. ESP-Board (d:\esp_vehicle\)
- Empfängt Befehle über UART (115200 baud)
- Parst das Format `"direction,speed"`
- Sendet den Befehl an **alle** Fahrzeuge gleichzeitig über ESP-NOW
- Fahrzeug-IDs werden im ESP automatisch vergeben (1-4)

### 3. Empfängerautos
- Empfangen die Befehle über ESP-NOW
- Führen die entsprechenden Bewegungen aus

## Command-Mapping

Die PC-Anwendung sendet folgende Richtungs-Codes:

| Code | Bedeutung | ESP Speed |
|------|-----------|-----------|
| 0    | Anhalten  | 0         |
| 1    | Vorwärts  | 150       |
| 2    | Rückwärts | 150       |
| 3    | Links     | 150       |
| 4    | Rechts    | 150       |
| 5    | Stopp     | 0         |

## Verwendung

### 1. ESP-Board vorbereiten
```bash
cd d:\esp_vehicle
idf.py build
idf.py flash
idf.py monitor
```

### 2. PC-Anwendung starten
```bash
cd "c:\Users\jonas\OneDrive\Documents\GitHub\PDS-T1000-TSA24"
.\main.exe
```

### 3. Serielle Verbindung herstellen
1. Das Kalibrierungs-Fenster öffnet sich automatisch
2. Im Bereich "ESP KOMUNIKATION":
   - COM-Port auswählen (wo das ESP angeschlossen ist)
   - "ESP Verbinden" klicken
   - Optional: "Auto-Send" aktivieren für automatisches Senden

### 4. Befehle senden
- **Manuell**: Button "Befehle Senden" klicken
- **Automatisch**: "Auto-Send" aktivieren - sendet automatisch bei jeder Aktualisierung

## Debugging

### ESP-Board Monitor
```bash
cd d:\esp_vehicle
idf.py monitor
```

Erwartete Ausgabe:
```
📡 Sende an ALLE Fahrzeuge: Direction=1, Speed=150
✅ Fahrzeug 1: Befehl gesendet
✅ Fahrzeug 2: Befehl gesendet
✅ Fahrzeug 3: Befehl gesendet
✅ Fahrzeug 4: Befehl gesendet
```

### PC-Anwendung Console
- Zeigt Verbindungsstatus
- Zeigt gesendete Befehle
- Zeigt Auto-Send Status

## Dateistruktur

### Neue Dateien:
- `include/serial_communication.h` - Header für serielle Kommunikation
- `src/serial_communication.cpp` - Implementation der seriellen Kommunikation

### Erweiterte Dateien:
- `src/test_window.cpp` - GUI für ESP-Kontrolle hinzugefügt
- `build.bat` - Build-Script um neue Datei erweitert

## Fehlerbehebung

### ESP nicht verbunden
- COM-Port prüfen (Geräte-Manager)
- ESP-Board Reset drücken
- Anderen COM-Port versuchen

### Befehle kommen nicht an
- ESP Monitor prüfen auf empfangene Daten
- Baudrate prüfen (115200)
- Verkabelung prüfen

### Auto-Send funktioniert nicht
- ESP-Verbindung zuerst herstellen
- vehicle_commands.json wird automatisch aktualisiert
- Fahrzeuge müssen vom System erkannt werden

## Technische Details

### Datenformat PC → ESP:
```
"1,150\n"  // Direction 1 (vorwärts), Speed 150
"5,0\n"    // Direction 5 (stopp), Speed 0
```

### Datenformat ESP → Fahrzeuge:
```c
typedef struct {
    int id;         // Fahrzeug-ID (1-4)
    int direction;  // Richtung (1-5)
    int speed;      // Geschwindigkeit (0-255)
    uint32_t timestamp;
} direction_data_t;
```

Das System ist jetzt bereit für die vollständige Fahrzeugsteuerung!

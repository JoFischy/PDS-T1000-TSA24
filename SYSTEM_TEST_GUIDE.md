# 🚗 VOLLSTÄNDIGER SYSTEM-TEST

## Schritt-für-Schritt Anleitung

### 1. ESP-Board vorbereiten
```bash
# Im ESP-Verzeichnis
cd d:\esp_vehicle
idf.py build
idf.py flash
idf.py monitor
```

**Erwartete ESP-Ausgabe:**
```
🚀 Starting ESP-NOW Direction/Speed Controller...
📶 WiFi initialized
📡 ESP-NOW initialized
✅ Fahrzeug 1 als Peer hinzugefügt: 74:4d:bd:a1:bf:04
✅ Fahrzeug 2 als Peer hinzugefügt: 48:ca:43:2e:34:44
✅ Fahrzeug 3 als Peer hinzugefügt: dc:da:0c:20:f2:64
✅ Fahrzeug 4 als Peer hinzugefügt: 74:4d:bd:a0:72:1c
✅ System initialized. Warte auf Direction/Speed-Befehle über UART...
```

### 2. PC-System starten
```bash
# Im Projekt-Verzeichnis
cd "c:\Users\jonas\OneDrive\Documents\GitHub\PDS-T1000-TSA24"
.\F5_Monitor2.bat
```

**Was passiert:**
1. Build läuft automatisch
2. Raylib-Fenster öffnet sich (Kamera + Pathfinding)
3. Windows API Fenster öffnet sich (Live-Anzeige)
4. Kalibrierungs-Fenster öffnet sich automatisch

### 3. ESP-Verbindung herstellen

Im **Kalibrierungs-Fenster**:

1. **COM-Port auswählen** (z.B. COM3, COM4, etc.)
2. **"ESP Verbinden"** klicken
3. Status sollte zeigen: **"Status: ESP verbunden!"**

### 4. Test: Manuelle Befehle

Im Kalibrierungs-Fenster:
- **"Befehle Senden"** klicken

**Erwartete ESP-Ausgabe:**
```
📥 Empfangen: Direction=1, Speed=150 (für ALLE Fahrzeuge)
✅ Befehl gültig -> sende an ALLE 4 Fahrzeuge
📡 Sende an ALLE Fahrzeuge: Direction=1, Speed=150
🚗 Sende an Fahrzeug 1: Direction=1, Speed=150
✅ Fahrzeug 1: Befehl gesendet
🚗 Sende an Fahrzeug 2: Direction=1, Speed=150
✅ Fahrzeug 2: Befehl gesendet
🚗 Sende an Fahrzeug 3: Direction=1, Speed=150
✅ Fahrzeug 3: Befehl gesendet
🚗 Sende an Fahrzeug 4: Direction=1, Speed=150
✅ Fahrzeug 4: Befehl gesendet
```

### 5. Auto-Send aktivieren

1. **"Auto-Send"** Checkbox aktivieren
2. System sendet automatisch bei jeder Fahrzeug-Bewegung

**Was passiert automatisch:**
- Kamera erkennt Fahrzeuge
- System berechnet notwendige Bewegungen
- `vehicle_commands.json` wird aktualisiert
- Befehle werden automatisch an ESP gesendet
- ESP leitet an alle Autos weiter

## Test-Szenarien

### Szenario 1: Fahrzeug Route setzen
1. Im Raylib-Fenster: **Fahrzeug anklicken** (wird magenta)
2. **Zielknoten anklicken**
3. Route wird berechnet und Befehle gesendet

### Szenario 2: Live vehicle_commands.json
```json
{
  "vehicles": [
    {
      "id": 2,
      "command": 1    // 1 = Vorwärts
    },
    {
      "id": 3, 
      "command": 3    // 3 = Links
    },
    {
      "id": 1,
      "command": 5    // 5 = Stopp
    }
  ]
}
```

### Szenario 3: ESP Kommunikation testen
```bash
# ESP Monitor zeigt empfangene Daten:
📤 Gesendet: 1,150
📤 Gesendet: 3,150  
📤 Gesendet: 5,0
```

## Fehlerbehebung

### ❌ ESP nicht gefunden
- USB-Kabel prüfen
- COM-Port im Geräte-Manager prüfen
- ESP Reset drücken

### ❌ Keine Fahrzeuge erkannt
- Kamera-Verbindung prüfen
- Beleuchtung anpassen
- Kalibrierung anpassen

### ❌ Befehle kommen nicht an
- ESP Monitor prüfen
- vehicle_commands.json prüfen
- Auto-Send Status prüfen

## System-Status prüfen

### PC-Console Ausgabe:
```
✅ Verbunden mit ESP-Board auf COM3 (115200 baud)
📡 Auto-Sende: Übertrage Befehle an ESP...
📤 Gesendet: 1,150
```

### ESP Monitor Ausgabe:
```
📥 Empfangen: Direction=1, Speed=150
📡 Sende an ALLE Fahrzeuge: Direction=1, Speed=150
✅ Befehle an alle 4 Fahrzeuge gesendet
```

Das System ist jetzt **VOLLSTÄNDIG FUNKTIONAL** für automatische Fahrzeugsteuerung! 🎉

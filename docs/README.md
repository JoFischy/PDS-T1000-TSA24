# PDS-T1000-TSA24 - Autonomes Fahrzeug System

Ein umfassendes System zur Simulation und Steuerung eines autonomen Fahrzeugs mit ESP32-Mikrocontroller und PC-basierter Simulation.

## 📋 Inhaltsverzeichnis

- [Überblick](#überblick)
- [System-Architektur](#system-architektur)
- [Komponenten](#komponenten)
- [Installation](#installation)
- [Verwendung](#verwendung)
- [Entwicklung](#entwicklung)
- [Hardware-Setup](#hardware-setup)
- [Troubleshooting](#troubleshooting)

## 🎯 Überblick

Das PDS-T1000-TSA24 Projekt ist ein autonomes Fahrzeugsystem, das aus mehreren Komponenten besteht:

- **ESP32-basierte Fahrzeugsteuerung** mit Live-Sensor-Integration
- **PC-basierte Simulation** mit Raylib-Grafik-Engine
- **Python-basierte Bilderkennung** für Farb- und Objekterkennung
- **UART-Kommunikation** zwischen ESP32 und PC
- **Koordinatenfilterung** für präzise Navigation

## 🏗️ System-Architektur

```
┌─────────────────┐    UART    ┌─────────────────┐
│     ESP32       │ ◄─────────► │    PC System    │
│  (Fahrzeug)     │             │   (Simulation)  │
├─────────────────┤             ├─────────────────┤
│ • Sensoren      │             │ • Raylib GUI    │
│ • Motoren       │             │ • Python Vision │
│ • WiFi/Bluetooth│             │ • Koordinaten   │
│ • Live-Daten    │             │ • Algorithmen   │
└─────────────────┘             └─────────────────┘
```

## 🧩 Komponenten

### ESP32 Fahrzeug (`esp_vehicle/`)
- **Hauptsteuerung**: ESP32-Mikrocontroller
- **Sensoren**: Ultraschall, Kamera, IMU
- **Aktuatoren**: Motoren, Servos
- **Kommunikation**: UART zu PC

### PC Simulation (`src/`, `include/`)
- **Grafik-Engine**: Raylib für 2D/3D Visualisierung
- **Fahrzeugsimulation**: Physik und Bewegungsmodelle
- **Koordinatensystem**: Präzise Positionierung
- **Bilderkennung**: Python-Integration für Computer Vision

### Externe Bibliotheken (`external/`)
- **Raylib**: Grafik und Game-Engine
- **Pybind11**: Python-C++ Integration

## 🚀 Installation

### Voraussetzungen
- Windows 10/11
- Visual Studio 2019+ oder MinGW
- Python 3.8+
- ESP-IDF Framework
- CMake 3.15+

### 1. Repository klonen
```powershell
git clone https://github.com/JoFischy/PDS-T1000-TSA24.git
cd PDS-T1000-TSA24
```

### 2. Abhängigkeiten installieren
```powershell
# Python-Pakete installieren
pip install opencv-python numpy matplotlib

# ESP-IDF Setup (falls nicht installiert)
# Folgen Sie der offiziellen ESP-IDF Installation
```

### 3. Projekt kompilieren
```powershell
# PC-Anwendung kompilieren
.\build.bat

# ESP32-Firmware kompilieren (optional)
cd esp_vehicle
idf.py build
```

## 🎮 Verwendung

### Schnellstart
```powershell
# System komplett starten
.\START_SYSTEM.bat

# Nur PC-Simulation
.\main.exe

# UART-Monitor für ESP32
.\F5_Monitor2.bat
```

### ESP32 Flash-Prozess
```powershell
# ESP32 Setup und Flash
.\SETUP_ESP32_LIVE.bat

# Oder manuell:
cd esp_vehicle
idf.py flash monitor
```

### Koordinaten-Management
- Koordinaten werden in `coordinates.json` gespeichert
- Automatische Filterung und Glättung der Bewegungsdaten
- Live-Tracking der Fahrzeugposition

## 🛠️ Entwicklung

### Projekt-Struktur
```
├── src/                    # C++ Hauptquellcode
│   ├── main.cpp           # Haupteinstiegspunkt
│   ├── car_simulation.cpp # Fahrzeugsimulation
│   ├── coordinate_filter.cpp # Koordinatenfilterung
│   ├── renderer.cpp       # Grafik-Rendering
│   └── Farberkennung.py   # Python-Bilderkennung
├── include/               # Header-Dateien
├── esp_vehicle/          # ESP32-Firmware
├── external/             # Externe Bibliotheken
└── build/               # Kompilierte Dateien
```

### Wichtige Dateien
- **`main.cpp`**: Hauptprogramm mit Raylib-Integration
- **`car_simulation.cpp`**: Fahrzeugphysik und -verhalten
- **`coordinate_filter.cpp`**: Koordinatenglättung und -filterung
- **`Farberkennung.py`**: Computer Vision für Objekterkennung
- **`Vehicle.h`**: Fahrzeugklassen-Definition

### Build-System
- **CMake**: Hauptbuild-System
- **`build.bat`**: Windows-Build-Skript
- **ESP-IDF**: ESP32-Build-System

## 🔌 Hardware-Setup

### ESP32 Verbindung
1. ESP32 über USB verbinden
2. COM-Port identifizieren
3. `SETUP_ESP32_LIVE.bat` ausführen
4. UART-Kommunikation testen

### Sensoren
- **Ultraschall**: Abstandsmessung
- **Kamera**: Bilderkennung und Navigation
- **IMU**: Orientierung und Bewegung
- **Encoder**: Raddrehung und Geschwindigkeit

## 🐛 Troubleshooting

### Häufige Probleme

**Kompilierungsfehler:**
```powershell
# Raylib nicht gefunden
# Lösung: Überprüfen Sie external/raylib/ Verzeichnis

# Python-Integration fehlgeschlagen
# Lösung: Pybind11 neu installieren
```

**ESP32-Verbindung:**
```powershell
# COM-Port nicht erkannt
# Lösung: Treiber installieren und Port überprüfen

# Flash-Fehler
# Lösung: ESP32 in Download-Modus versetzen
```

**Laufzeit-Probleme:**
```powershell
# Koordinaten.json nicht gefunden
# Lösung: Datei wird automatisch erstellt

# Python-Skript Fehler
# Lösung: OpenCV und NumPy installieren
```

### Debug-Modi
```powershell
# Verbose-Ausgabe
.\main.exe --debug

# UART-Monitor mit Logging
.\F5_Monitor2.bat > debug.log
```

## 📊 Konfiguration

### Koordinaten-System
- **Format**: JSON mit X/Y-Koordinaten
- **Filterung**: Kalman-Filter für Glättung
- **Auflösung**: Millimeter-Genauigkeit

### Kommunikation
- **UART**: 115200 Baud
- **Protokoll**: Custom binary/text hybrid
- **Latenz**: <10ms für Echtzeitsteuerung

## 🤝 Beitragen

1. Fork des Repositories erstellen
2. Feature-Branch erstellen (`git checkout -b feature/NeuesFunktion`)
3. Änderungen committen (`git commit -am 'Neue Funktion hinzugefügt'`)
4. Branch pushen (`git push origin feature/NeuesFunktion`)
5. Pull Request erstellen

## 📄 Lizenz

Dieses Projekt ist Teil des PDS-Kurses TSA24. Alle Rechte vorbehalten.

## 👥 Autoren

- **JoFischy** - Hauptentwickler
- TSA24 Team - Mitwirkende

## 📞 Support

Bei Fragen oder Problemen:
1. Issues auf GitHub erstellen
2. Dokumentation überprüfen
3. Debug-Logs analysieren

---

**Letztes Update**: August 2025  
**Version**: 1.0.0  
**Branch**: glättung

# PDS-T1000-TSA24 🚗📡

**Professionelles Fahrzeug-Tracking und Simulations-System**

Ein hochmodernes Computer Vision System zur Echtzeitüberwachung und -simulation von Fahrzeugen mit ESP32-Integration, entwickelt für das TSA24-Projekt.

---

## 🌟 Projektübersicht

Das PDS-T1000-TSA24 ist ein vollständiges autonomes Fahrzeug-Tracking-System, das Kameras, ESP32-Mikrocontroller und eine leistungsstarke C++/Python-Simulation kombiniert. Das System erkennt und verfolgt Fahrzeuge in Echtzeit und bietet eine umfassende Visualisierung und Steuerung.

### ✨ Hauptfunktionen

- **🎯 Echtzeit-Objekterkennung**: Computer Vision mit OpenCV für präzise Fahrzeugerkennung
- **📡 ESP32-Integration**: Drahtlose Kommunikation und Sensorik
- **🖥️ Raylib-Visualisierung**: Hochperformante 2D/3D-Grafik-Engine
- **🔧 Multi-Monitor-Support**: Automatische Vollbild-Darstellung auf mehreren Monitoren
- **📊 Koordinatenfilterung**: Kalman-Filter für glatte Bewegungsdaten
- **🛡️ Robustes Protokoll**: UART-basierte Kommunikation mit Fehlerbehandlung

### 🎮 Anwendungsbereiche

- **Autonome Fahrzeuge**: Entwicklung und Test von Navigationsalgorithmen
- **Verkehrssimulation**: Realistische Fahrzeugbewegungen und -interaktionen
- **Robotik-Forschung**: Computer Vision und Sensorfusion
- **Bildungsbereich**: Demonstration von KI und Robotik-Konzepten

---

## 🚀 Schnellstart

### 1. Systemstart

```powershell
# Vollständiges System auf Monitor 2 starten
.\F5_Monitor2.bat

# Oder manuell kompilieren und starten
.\build.bat
.\main.exe --monitor2
```

### 2. ESP32-Setup

```powershell
# ESP32 automatisch einrichten
.\SETUP_ESP32_LIVE.bat

# Oder manuell (im esp_vehicle/ Verzeichnis)
idf.py build flash monitor
```

### 3. System verwenden

- **ESC**: System beenden
- **+/-**: Toleranz-Einstellungen anpassen
- **Automatisch**: Kamera-Tracking startet sofort

---

## 🏗️ Systemarchitektur

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Kamera-Feed   │───▶│  Computer Vision │───▶│  Koordinaten-   │
│   (OpenCV)      │    │   (Python)       │    │   filter        │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                                          │
┌─────────────────┐    ┌──────────────────┐    ┌─────────▼─────────┐
│     ESP32       │◀──▶│  UART Protocol   │◀──▶│  Raylib Engine    │
│   (Hardware)    │    │  (Kommunikation) │    │  (Visualisierung) │
└─────────────────┘    └──────────────────┘    └───────────────────┘
```

### 📁 Projektstruktur

```
PDS-T1000-TSA24/
├── 📄 Dokumentation
│   ├── README.md              # Hauptdokumentation (diese Datei)
│   ├── DEVELOPER_GUIDE.md     # Entwickler-Handbuch
│   ├── TECHNICAL_DOCS.md      # Technische Spezifikationen
│   └── ESP32_SETUP_GUIDE.md   # ESP32-Setup Anleitung
├── 🔧 Build-System
│   ├── build.bat              # Windows-Build-Skript
│   ├── F5_Monitor2.bat        # Schnellstart Monitor 2
│   └── START_SYSTEM.bat       # System-Starter
├── 💻 C++ Hauptanwendung
│   ├── src/                   # Quellcode
│   │   ├── main.cpp           # Hauptprogramm
│   │   ├── car_simulation.cpp # Fahrzeugphysik
│   │   ├── py_runner.cpp      # Python-Integration
│   │   ├── coordinate_filter.cpp # Kalman-Filter
│   │   └── Farberkennung.py   # Computer Vision
│   ├── include/               # Header-Dateien
│   └── external/              # Externe Bibliotheken
├── 🔌 ESP32-Firmware
│   └── esp_vehicle/           # ESP-IDF Projekt
├── 📊 Daten und Konfiguration
│   └── coordinates.json       # Koordinaten-Cache
└── 🛠️ Build-Artefakte
    └── build/                 # Kompilierte Dateien
```

---

## ⚙️ System-Anforderungen

### Hardware-Anforderungen

| Komponente | Minimum | Empfohlen |
|------------|---------|-----------|
| **CPU** | Intel i5 / AMD Ryzen 5 | Intel i7 / AMD Ryzen 7 |
| **RAM** | 8 GB | 16 GB |
| **GPU** | DirectX 11 kompatibel | Dedizierte Grafikkarte |
| **Storage** | 2 GB freier Speicher | SSD empfohlen |
| **USB** | USB 2.0 Port | USB 3.0 für ESP32 |
| **Kamera** | USB-Webcam | HD-Webcam (1080p) |

### Software-Anforderungen

- **Betriebssystem**: Windows 10/11 (64-bit)
- **Compiler**: MinGW-w64 oder Visual Studio 2019+
- **Python**: 3.8 - 3.11
- **ESP-IDF**: v4.4+ (für ESP32-Entwicklung)
- **Git**: Für Repository-Management

---

## 🎯 Funktionen im Detail

### 🔍 Computer Vision Engine

- **Mehrfarbenerkennung**: Bis zu 5 verschiedene Fahrzeugtypen
- **Adaptive Schwellwerte**: Automatische Helligkeitsanpassung
- **Crop-Region**: Flexibler Bildausschnitt für optimale Performance
- **Outlier-Detection**: Filterung von falschen Erkennungen

### 🚗 Fahrzeugsimulation

- **Realistische Physik**: Kinematisches Fahrzeugmodell
- **Kollisionserkennung**: Präzise Rechteck-basierte Kollisionen
- **Multi-Fahrzeug**: Gleichzeitige Verfolgung mehrerer Objekte
- **Performance-Optimiert**: 60 FPS bei Full-HD Auflösung

### 📡 Kommunikation

- **UART-Protokoll**: Robuste serielle Kommunikation
- **Nachrichtentypen**: Position, Sensor, Befehle, Status
- **CRC-Prüfsummen**: Datenintegrität gewährleistet
- **Async-Verarbeitung**: Non-blocking Kommunikation

---

## 🛠️ Installation und Setup

### Automatische Installation

```powershell
# 1. Repository klonen
git clone https://github.com/JoFischy/PDS-T1000-TSA24.git
cd PDS-T1000-TSA24

# 2. Abhängigkeiten installieren (automatisch)
.\setup.bat

# 3. System bauen und starten
.\F5_Monitor2.bat
```

### Manuelle Installation

<details>
<summary>📋 Manuelle Schritte anzeigen</summary>

#### 1. Compiler Setup

```powershell
# MinGW-w64 installieren
winget install mingw-w64

# Oder über Chocolatey
choco install mingw
```

#### 2. Python-Abhängigkeiten

```powershell
pip install opencv-python numpy matplotlib
```

#### 3. Raylib-Integration

Die Raylib-Bibliothek ist bereits im `external/` Verzeichnis enthalten.

#### 4. Build

```powershell
.\build.bat
```

</details>

---

## 🎮 Verwendung

### Basis-Kommandos

```powershell
# Standard-Fenstermodus
.\main.exe

# Vollbild auf aktuellem Monitor
.\main.exe --fullscreen

# Vollbild auf Monitor 2 (für Präsentationen)
.\main.exe --monitor2

# Hilfe anzeigen
.\main.exe --help
```

### Keyboard-Steuerung

| Taste | Funktion |
|-------|----------|
| **ESC** | System beenden |
| **+** | Toleranz erhöhen |
| **-** | Toleranz reduzieren |
| **F11** | Vollbild umschalten |
| **F1** | Debug-Informationen |

### ESP32-Befehle

```powershell
# ESP32 Code kompilieren
cd esp_vehicle
idf.py build

# Auf ESP32 flashen
idf.py flash

# Serial Monitor
idf.py monitor

# Alles in einem Schritt
idf.py build flash monitor
```

---

## 📈 Performance-Tipps

### System-Optimierung

1. **CPU-Priorisierung**: Task-Manager → Details → main.exe → Hohe Priorität
2. **GPU-Beschleunigung**: Aktivierung der Hardware-Beschleunigung
3. **Kamera-Auflösung**: Niedrigere Auflösung für bessere Performance
4. **Multi-Threading**: System nutzt automatisch alle CPU-Kerne

### Monitoring

```cpp
// Performance-Metriken im Debug-Modus
FPS: 60.0
Frame Time: 16.7ms
Objects Detected: 3
UART Messages/sec: 50
```

---

## 🐛 Troubleshooting

### Häufige Probleme

<details>
<summary>🔧 Build-Fehler</summary>

**Problem**: `g++: command not found`
```powershell
# Lösung: MinGW-w64 installieren
winget install mingw-w64
# PATH-Variable überprüfen
echo $env:PATH
```

**Problem**: Python-Bibliotheken fehlen
```powershell
# Lösung: Abhängigkeiten installieren
pip install -r requirements.txt
```

</details>

<details>
<summary>📹 Kamera-Probleme</summary>

**Problem**: Kamera nicht erkannt
- USB-Kabel prüfen
- Andere Programme schließen (Skype, Teams, etc.)
- Kamera-Permissions überprüfen

**Problem**: Schlechte Erkennung
- Beleuchtung verbessern
- Crop-Region anpassen
- Farb-Toleranzen justieren

</details>

<details>
<summary>🔌 ESP32-Probleme</summary>

**Problem**: ESP32 nicht erkannt
```powershell
# COM-Ports anzeigen
Get-WmiObject -Class Win32_SerialPort
```

**Problem**: Flash-Fehler
```powershell
# Bootloader-Modus aktivieren
# BOOT-Taste gedrückt halten + Reset
```

</details>

---

## 🤝 Beitragen

Wir freuen uns über Beiträge zur Verbesserung des Systems!

### Development-Workflow

```bash
# 1. Fork erstellen
git fork https://github.com/JoFischy/PDS-T1000-TSA24.git

# 2. Feature-Branch
git checkout -b feature/neue-funktion

# 3. Entwickeln und testen
# ... Code-Änderungen ...

# 4. Pull Request erstellen
git push origin feature/neue-funktion
```

### Code-Standards

- **C++17**: Moderner C++ Standard
- **PEP 8**: Python Style Guide
- **ESP-IDF**: Espressif Coding Guidelines
- **Kommentare**: Deutsch für projektspezifische, Englisch für generische Funktionen

---

## 📝 Dokumentation

| Dokument | Zielgruppe | Inhalt |
|----------|------------|--------|
| **README.md** | Alle Nutzer | Projektübersicht und Schnellstart |
| **DEVELOPER_GUIDE.md** | Entwickler | API, Build-System, Testing |
| **TECHNICAL_DOCS.md** | Techniker | Architektur, Protokolle, Performance |
| **ESP32_SETUP_GUIDE.md** | Hardware | ESP32-Setup, Flashing, Debugging |

---

## 🏆 Features und Roadmap

### ✅ Implementiert

- ✅ Echtzeit-Objekterkennung
- ✅ Multi-Monitor-Support
- ✅ ESP32-UART-Kommunikation
- ✅ Kalman-Filter-Glättung
- ✅ Raylib-Rendering-Engine
- ✅ Python-C++ Integration

### 🚧 In Entwicklung

- 🚧 Web-Interface für Remote-Monitoring
- 🚧 Machine Learning Objektklassifikation
- 🚧 3D-Visualisierung
- 🚧 Pfadplanung-Algorithmen

### 🔮 Geplant

- 🔮 ROS2-Integration
- 🔮 Docker-Container
- 🔮 Cloud-Synchronisation
- 🔮 Mobile App

---

## 📄 Lizenz

Dieses Projekt ist unter der MIT-Lizenz lizenziert. Siehe [LICENSE](LICENSE) für Details.

---

## 📞 Support

**Discord**: TSA24-Development-Server  
**E-Mail**: support@tsa24-project.de  
**Issues**: [GitHub Issues](https://github.com/JoFischy/PDS-T1000-TSA24/issues)

---

## 👥 Team

- **Projektleitung**: Jonas Fischer
- **Hardware**: ESP32-Team
- **Software**: C++/Python-Team
- **Computer Vision**: CV-Team

---

<div align="center">

**🌟 Wenn dir das Projekt gefällt, gib uns einen Stern! ⭐**

[![Stars](https://img.shields.io/github/stars/JoFischy/PDS-T1000-TSA24?style=social)](https://github.com/JoFischy/PDS-T1000-TSA24/stargazers)
[![Forks](https://img.shields.io/github/forks/JoFischy/PDS-T1000-TSA24?style=social)](https://github.com/JoFischy/PDS-T1000-TSA24/network)
[![Issues](https://img.shields.io/github/issues/JoFischy/PDS-T1000-TSA24)](https://github.com/JoFischy/PDS-T1000-TSA24/issues)

---

**Entwickelt mit ❤️ von Team TSA24 🚀**

*Version 2.0.0 | Letztes Update: August 2025*

</div>
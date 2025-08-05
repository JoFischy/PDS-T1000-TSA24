# PDS-T1000-TSA24 - Kamera-Tracking-System mit Monitor 2 Vollbild

## 🚀 Schnellstart

**Einfachster Start: F5_Monitor2.bat ausführen**

```bash
.\F5_Monitor2.bat
```

Das System startet automatisch:
- ✅ Kompiliert C++ Programm mit Python-Integration
- ✅ Startet Raylib-Fenster im Vollbild auf **Monitor 2** (1920x1200)
- ✅ Aktiviert automatisch das Kamera-System
- ✅ Zeigt Echtzeit-Objekterkennung an

## 📋 Start-Optionen

### 🎯 Hauptverwendung:
```bash
# Vollbild Monitor 2 (Empfohlen für Projektion)
.\F5_Monitor2.bat

# Alternativ manuell:
.\build.bat
.\main.exe --monitor2
```

### 🖥️ Andere Modi:
```bash
# Normal (Fenster auf Monitor 1)
.\build.bat
.\main.exe

# Vollbild auf aktuellem Monitor
.\main.exe --fullscreen

# Hilfe anzeigen
.\main.exe --help
```

## 🏗️ System-Architektur

### Core Components:
- **`src/main.cpp`** - Raylib-Anwendung mit Monitor-Management
- **`src/py_runner.cpp`** - Python-C++ Bridge für Kamera-Integration  
- **`src/Farberkennung.py`** - Kamera-basierte Objekterkennung
- **`src/car_simulation.cpp`** - Koordinaten-Transformation und Visualisierung

### Build System:
- **`build.bat`** - Kompiliert C++ mit g++, Python 3.11, Raylib
- **`F5_Monitor2.bat`** - Ein-Klick Build & Start für Monitor 2

## 🎨 Funktionen

### 📍 Koordinaten-System:
- **Kamera-Input**: Python OpenCV Objekterkennung
- **Transformation**: Kamera → Fenster-Koordinaten  
- **Visualisierung**: Raylib mit weißem Hintergrund
- **Vollbild**: 1920x1200 auf Monitor 2 für Projektion

### 🎯 Objekterkennung:
- **Multi-Color Detection**: 5 verschiedene Farben (Front, Heck1-4)
- **HSV-basiert**: Robust gegen Lichtverhältnisse
- **Auto-Pairing**: Intelligente Zuordnung von Front/Heck-Punkten zu Fahrzeugen
- **Echtzeit**: 60 FPS Raylib + Kamera-Updates

### 🖼️ Anzeige-Features:
- **Weißer Vollbild-Hintergrund**: Optimal für Projektion
- **Objekt-Darstellung**: Farbige Punkte und Fahrzeug-Linien
- **Koordinaten-Anzeige**: Debug-Informationen
- **ESC**: Programm beenden

## 🔧 Technische Details

### Dependencies:
- **C++17** mit g++ Compiler
- **Python 3.11** (im PATH verfügbar)
- **Raylib 5.6-dev** (in `external/raylib/`)
- **OpenCV** (via Python pip)

### Koordinaten-Transformation:
```cpp
// Vollbild-Transformation
field_transform.field_width = currentWidth;   // 1920px
field_transform.field_height = currentHeight; // 1200px
field_transform.offset_x = 0;                 // Gesamte Fläche
field_transform.offset_y = 0;                 // Gesamte Fläche
```

### Monitor-Management:
- **Automatic Detection**: Erkennt verfügbare Monitore
- **Position Control**: `SetWindowPosition(1920, 0)` für Monitor 2
- **Fullscreen Mode**: `FLAG_WINDOW_UNDECORATED` für Vollbild

## 📁 Projekt-Struktur

```
PDS-T1000-TSA24/
├── src/
│   ├── main.cpp              # Raylib Hauptprogramm
│   ├── py_runner.cpp         # Python-C++ Integration  
│   ├── car_simulation.cpp    # Koordinaten & Visualisierung
│   └── Farberkennung.py      # Kamera-Objekterkennung
├── include/
│   ├── car_simulation.h      # Header Definitionen
│   ├── py_runner.h           # Python Bridge Header
│   └── Vehicle.h             # Datenstrukturen
├── external/raylib/          # Raylib Graphics Library
├── build.bat                 # C++ Compiler Script
├── F5_Monitor2.bat          # Ein-Klick Start für Monitor 2
└── .vscode/                  # VS Code Konfiguration
```

## 🎮 Bedienung

### Im Programm:
- **ESC** - Programm beenden
- **Fenster schließen** - Programm beenden
- **Keine Tasten nötig** - Alles läuft automatisch

### Kamera-System:
- **Automatische Initialisierung** beim Start
- **Dynamische Objekterkennung** in Echtzeit
- **Farb-basierte Erkennung** (HSV-Werte konfigurierbar)

## 🚨 Problemlösung

### Häufige Probleme:

**Monitor 2 nicht verfügbar:**
```bash
# Fallback auf Monitor 1
.\main.exe --fullscreen
```

**Build-Fehler:**
```bash
# Python 3.11 installiert?
python --version

# Raylib verfügbar?
dir external\raylib\src\raylib.lib
```

**Kamera-Probleme:**
```bash
# Test der Kamera direkt
python src/Farberkennung.py
```

## 📊 Performance

- **Raylib**: 60 FPS konstant
- **Kamera**: Abhängig von USB-Bandbreite  
- **Latenz**: < 50ms Ende-zu-Ende
- **Auflösung**: 1920x1200 Vollbild ohne Performance-Verlust

---

**Entwickelt für präzise Fahrzeug-Tracking mit Projektor-Setup auf Monitor 2**


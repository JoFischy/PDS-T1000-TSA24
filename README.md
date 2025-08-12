# PDS-T1000-TSA24 🚗📡

**Performance-optimiertes Fahrzeug-Tracking System**

Ein hochperformantes Computer Vision System zur Echtzeitüberwachung von Fahrzeugen mit minimaler Latenz für das TSA24-Projekt.

---

## 🌟 Projektübersicht

Das PDS-T1000-TSA24 ist ein optimiertes Fahrzeug-Tracking-System, das Kameras und eine leistungsstarke C++/Python-Simulation kombiniert. Das System erkennt und verfolgt Fahrzeuge in Echtzeit mit minimaler Verzögerung zwischen Kameraerkennung und Visualisierung.

### ✨ Hauptfunktionen

- **🎯 Echtzeit-Objekterkennung**: Computer Vision mit OpenCV für präzise Fahrzeugerkennung
- **🖥️ Raylib-Visualisierung**: Hochperformante 2D-Grafik-Engine
- **🔧 Multi-Monitor-Support**: Automatische Vollbild-Darstellung auf mehreren Monitoren
- **⚡ Performance-Optimierung**: Minimale Latenz durch FastCoordinateFilter
- **🎮 Farbfilter-Toggle**: F-Taste für Kalibriermodus mit Filterfenstern

### 🎮 Anwendungsbereiche

- **Autonome Fahrzeuge**: Entwicklung und Test von Navigationsalgorithmen
- **Verkehrssimulation**: Realistische Fahrzeugbewegungen und -interaktionen
- **Computer Vision**: Echtzeitverarbeitung von Kameradaten

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

### 2. Performance-Modus

Das System startet standardmäßig im Performance-Modus mit minimaler Latenz:
- **F-Taste**: Toggle zwischen Performance- und Kalibriermodus
- **ESC**: Beenden
- **+/-**: HSV-Toleranz anpassen

### 3. Kalibrierung

Im Kalibriermodus (F-Taste drücken) werden die HSV-Filterfenster angezeigt:
- Separate Fenster für jede Farberkennung
- Echtzeit-Anpassung der HSV-Werte
- Optimierung der Erkennungsgenauigkeit

---

## ⚙️ Technische Details

### Performance-Optimierungen

- **FastCoordinateFilter**: Direkte Durchleitung ohne komplexe Filterung (~5-10ms Latenz)
- **Release-Build**: Kompilierung mit -O3 -DNDEBUG Flags
- **Reduzierte Debug-Ausgaben**: Minimaler Overhead im Produktivbetrieb
- **Optimierte JSON-Serialisierung**: Kompakte Datenübertragung zwischen Python und C++

### Architektur

- **Frontend**: Raylib für 60 FPS Rendering
- **Backend**: OpenCV für HSV-basierte Farberkennung
- **Communication**: JSON-basierter Datenaustausch
- **Coordination**: FastCoordinateFilter für minimale Latenz

---

## 📁 Projektstruktur

```
PDS-T1000-TSA24/
├── src/                     # C++ Quellcode
│   ├── main.cpp            # Hauptprogramm
│   ├── car_simulation.cpp  # Simulationslogik
│   ├── coordinate_filter_fast.cpp  # Performance-Filter
│   ├── Farberkennung.py    # Python Farberkennung
│   └── renderer.cpp        # Raylib Rendering
├── include/                # Header-Dateien
├── build/                  # Kompilierte Dateien
├── external/raylib/        # Raylib Bibliothek
├── build.bat              # Build-Skript
├── F5_Monitor2.bat        # Quick-Start Skript
└── coordinates.json       # Koordinaten-Austausch
```

---

## 🔧 Entwicklung

### Build-System

```powershell
# Release-Build mit Optimierungen
.\build.bat

# Debug-Informationen verfügbar in build/
```

### Konfiguration

- HSV-Werte werden zur Laufzeit angepasst
- Monitor-Auswahl über Kommandozeilenparameter
- Performance-Toggle über F-Taste

---

## 📊 Performance-Metriken

- **Latenz**: ~5-10ms (Kamera → Raylib)
- **Framerate**: 60 FPS konstant
- **CPU-Usage**: Optimiert für Echtzeit-Performance
- **Memory**: Minimaler Footprint durch FastFilter

---

## 🚀 Zukunftserweiterungen

- Weitere Optimierungen der Renderingpipeline
- Zusätzliche Kalibrierungsoptionen
- Erweiterte Multi-Monitor-Unterstützung

---

*Entwickelt für TSA24 - Performance First Approach* ⚡

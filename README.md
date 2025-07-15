# 🚗 Multi-Vehicle Fleet Detection System

Ein intelligentes Kamera-basiertes Erkennungssystem für 4 Fahrzeuge mit einheitlicher Frontfarbe und individuellen Heckfarben.

## 🎯 Features

- **4-Fahrzeug-Erkennung** mit intelligenter Zuordnung
- **Einheitliche gelbe Frontfarbe** für alle Fahrzeuge
- **4 verschiedene Heckfarben**: Rot, Blau, Grün, Lila
- **Intelligenter Pairing-Algorithmus** für nächste Farbpunkte
- **2x2 Grid Display** mit individuellen Fahrzeugpanels
- **Echtzeit-Kompass-Visualisierung** für jedes Fahrzeug
- **Live-Status** und Positionsanzeige

## 🔧 Technische Architektur

- **C++17** mit Raylib 5.0 für Visualisierung
- **Python 3.13** mit OpenCV 4.12 für Computer Vision
- **CMake 3.28** Build-System
- **pybind11** für Python-C++ Integration

## 🚀 Quick Start

### Voraussetzungen
- CMake ≥ 3.28.0
- Python 3.13.x mit OpenCV
- Visual Studio 2022 oder kompatibler C++17-Compiler

### Build & Run
```bash
# Repository klonen
git clone https://github.com/JoFischy/PDS-T1000-TSA24.git
cd PDS-T1000-TSA24

# Pfade anpassen (siehe Setup-Anleitung)
# Dann kompilieren:
cmake -S . -B build
cmake --build build --config Debug

# Ausführen
./build/Debug/camera_detection.exe
```

## 📋 Setup für Team-Kollegen

**⚠️ WICHTIG**: System verwendet jetzt vereinfachte Datenstruktur!

Siehe `SETUP.md` für detaillierte Anweisungen.

## 🎮 Fahrzeug-Konfiguration

Das System erkennt 4 Fahrzeuge mit folgender Farbkodierung:

| Fahrzeug | Frontfarbe | Heckfarbe | Panel-Position |
|----------|------------|-----------|----------------|
| Auto 1   | Gelb       | Rot       | Oben Links     |
| Auto 2   | Gelb       | Blau      | Oben Rechts    |
| Auto 3   | Gelb       | Grün      | Unten Links    |
| Auto 4   | Gelb       | Lila      | Unten Rechts   |

## 🧠 Intelligente Zuordnung

Das System verwendet einen Distance-Based Assignment Algorithmus:
- Erkennt alle gelben Frontpunkte
- Erkennt alle farbigen Heckpunkte (Rot, Blau, Grün, Lila)
- Ordnet die nächstgelegenen Paare zu (max. 200px Abstand)
- Verhindert Kollisionen durch intelligente Paarung

## 📊 Display Layout

```
┌─────────────┬─────────────┐
│   Auto 1    │   Auto 2    │
│   (Rot)     │   (Blau)    │
├─────────────┼─────────────┤
│   Auto 3    │   Auto 4    │
│   (Grün)    │   (Lila)    │
└─────────────┴─────────────┘
```

Jedes Panel zeigt:
- **Hauptposition** (X, Y) - Schwerpunkt des Fahrzeugs
- **Richtungswinkel** mit Kompass-Visualisierung
- **Status** (Erkannt/Nicht erkannt)
- **Fahrzeuggröße** (Distanz in Pixeln)
- **Heckfarbe** zur Identifikation

## 🔍 Computer Vision Details

### HSV-Farbfilterung
- **Gelb (Front)**: H: 20-30, S: 100-255, V: 100-255
- **Rot (Heck)**: H: 0-10 & 170-180, S: 100-255, V: 100-255
- **Blau (Heck)**: H: 100-130, S: 100-255, V: 100-255
- **Grün (Heck)**: H: 50-80, S: 100-255, V: 100-255
- **Lila (Heck)**: H: 130-160, S: 100-255, V: 100-255

### Bildverarbeitung
- Morphologische Operationen zur Rauschreduzierung
- Konturerkennung für präzise Farbpunkt-Lokalisierung
- **Distance-Based Pairing**: Intelligente Zuordnung nächster Farbpaare
- **Schwerpunkt-Berechnung**: Hauptposition zwischen Front- und Heckpunkt
- **Vereinfachte Datenstruktur**: Nur Hauptposition + Distanz

## 🏗️ Projektstruktur

```
├── src/
│   ├── main.cpp                    # Hauptprogramm
│   ├── MultiCarDisplay.cpp         # Raylib UI-System
│   ├── VehicleFleet.cpp           # Fahrzeugflotten-Management
│   ├── py_runner.cpp              # Python-C++ Bridge
│   └── MultiVehicleKamera.py      # Computer Vision Engine
├── include/
│   ├── Vehicle.h                  # Vereinfachte Datenstrukturen (Header-Only)
│   ├── MultiCarDisplay.h          # UI-Header
│   ├── VehicleFleet.h            # Fleet-Management
│   └── py_runner.h               # Python-Bridge
├── build/                         # Build-Artefakte (ignoriert)
└── CMakeLists.txt                 # Build-Konfiguration
```

## 🤝 Contribution

1. Fork das Repository
2. Feature-Branch erstellen (`git checkout -b feature/amazing-feature`)
3. Änderungen committen (`git commit -m 'Add amazing feature'`)
4. Branch pushen (`git push origin feature/amazing-feature`)
5. Pull Request öffnen

## 📄 License

Dieses Projekt steht unter der MIT License - siehe `LICENSE` Datei für Details.

## 🆘 Troubleshooting

### Häufige Probleme:
- **Kompilierungsfehler**: Pfade in CMakeLists.txt und py_runner.cpp anpassen
- **Python-Import-Fehler**: OpenCV installieren (`pip install opencv-python`)
- **Kamera nicht erkannt**: USB-Kamera anschließen und Treiber prüfen

Siehe `SETUP.md` für detaillierte Problemlösungen.

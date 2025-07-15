# 🔧 Setup-Anleitung für Multi-Vehicle Fleet System

Diese Anleitung hilft Team-Kollegen beim Setup des 4-Fahrzeug-Erkennungssystems.

## 📋 Voraussetzungen

### 1. CMake installieren
- **Version**: ≥ 3.28.0 (empfohlen: gleiche Version wie Entwickler)
- Download: [cmake.org/download](https://cmake.org/download/)
- Bei Installation: "Add CMake to system PATH" aktivieren

### 2. Python 3.13.x installieren
- **Version**: 3.13.5 (gleiche Version für Kompatibilität)
- Download: [python.org](https://www.python.org/downloads/)
- **Wichtig**: "Add Python to PATH" aktivieren
- **Wichtig**: "py launcher" aktivieren

### 3. Python-Pakete installieren
```bash
pip install opencv-python==4.12.0.88
pip install numpy
```

### 4. Visual Studio 2022
- Community Edition reicht aus
- **Workload**: "Desktop development with C++"
- CMake Tools sind automatisch enthalten

## 🚀 Projekt-Setup

### 1. Repository klonen
```bash
git clone https://github.com/JoFischy/PDS-T1000-TSA24.git
cd PDS-T1000-TSA24
```

### 2. ⚠️ KRITISCH: Lokale Pfade anpassen

**Du MUSST diese 4 Dateien an deine lokale Umgebung anpassen:**

#### A) `src/py_runner.cpp` - Zeile 26:
```cpp
// VORHER (Jonas' Pfad):
std::string src_path = "C:/Users/jonas/OneDrive/Documents/GitHub/PDS-T1000-TSA24/src";

// NACHHER (dein Pfad):
std::string src_path = "C:/Users/DEINNAME/Pfad/zum/Projekt/PDS-T1000-TSA24/src";
```

#### B) `CMakeLists.txt` - Zeilen 13-14:
```cmake
# VORHER (Jonas' Python):
include_directories("C:/Users/jonas/AppData/Local/Programs/Python/Python313/include")
link_directories("C:/Users/jonas/AppData/Local/Programs/Python/Python313/libs")

# NACHHER (deine Python-Installation):
include_directories("C:/Users/DEINNAME/AppData/Local/Programs/Python/Python313/include")
link_directories("C:/Users/DEINNAME/AppData/Local/Programs/Python/Python313/libs")
```

#### C) `CMakeLists.txt` - Zeile 49:
```cmake
# Falls deine Python-Version anders ist:
target_link_libraries(camera_detection PRIVATE raylib python313)
# Ändere python313 zu deiner Version (z.B. python312)
```

#### D) `.vscode/c_cpp_properties.json` - Zeile 9 (optional für IntelliSense):
```json
// VORHER:
"C:/Users/jonas/AppData/Local/Programs/Python/Python313/include"

// NACHHER:
"C:/Users/DEINNAME/AppData/Local/Programs/Python/Python313/include"
```

### 3. Python-Pfad finden
Wenn du deinen Python-Pfad nicht kennst:
```bash
python -c "import sys; print(sys.executable)"
```

### 4. Build-Prozess
```bash
# CMake konfigurieren
cmake -S . -B build

# Kompilieren
cmake --build build --config Debug
```

### 5. Programm starten
```bash
# Windows:
.\build\Debug\camera_detection.exe

# Alternative:
cd build\Debug
.\camera_detection.exe
```

## ✅ Erfolgreiche Installation testen

### Programm startet ohne Fehler:
- ✅ **Raylib-Fenster** öffnet sich (1000x700px)
- ✅ **2x2 Grid** mit 4 Fahrzeug-Panels (Auto-1 bis Auto-4)
- ✅ **OpenCV-Kamerafenster** "Multi-Vehicle Detection - Vereinfachte Darstellung" 
- ✅ **Konsole zeigt**: "Multi-Vehicle Kamera initialisiert"
- ✅ **Fahrzeug-Konfiguration**: Gelb vorne, verschiedene Heckfarben

### Fehler-Diagnose:

#### "Python interpreter failed to initialize"
→ Python-Pfade in CMakeLists.txt falsch

#### "Failed to load MultiVehicleKamera.py"
→ src_path in py_runner.cpp falsch

#### "opencv not found"
→ `pip install opencv-python` ausführen

#### Kompilierungsfehler
→ CMake-Version prüfen (≥ 3.28.0)

## 🎮 System verwenden

### Fahrzeug-Setup:
1. **4 Fahrzeuge** mit gelben Frontmarkierungen
2. **Verschiedene Heckfarben**: Rot, Blau, Grün, Lila
3. **Kamera positionieren** für gute Sicht auf alle Fahrzeuge

### Erwartete Ausgabe:
- **Raylib-Fenster**: 2x2 Grid mit 4 Fahrzeug-Panels
- **OpenCV-Kamerafenster**: "Multi-Vehicle Detection - Vereinfachte Darstellung"
- **Fahrzeugdarstellung**: Hauptposition als großer Kreis mit Richtungspfeil
- **Status-Informationen**: Position, Winkel, Distanz pro Fahrzeug
- **Vereinfachte Struktur**: Nur Hauptposition + Fahrzeuggröße

## 🔧 Erweiterte Konfiguration

### Kamera-Index ändern (falls mehrere Kameras):
In `MultiVehicleKamera.py` - Zeile ~20:
```python
cap = cv2.VideoCapture(0)  # Ändere 0 zu 1, 2, etc.
```

### HSV-Werte anpassen (bei anderen Lichtverhältnissen):
In `MultiVehicleKamera.py` - Funktionen `find_all_color_centers()` für Farberkennung

## 🆘 Support

Bei Problemen:
1. Prüfe alle 4 Pfad-Anpassungen
2. Stelle sicher, dass Python 3.13.x installiert ist
3. Vergewissere dich, dass OpenCV funktioniert: `python -c "import cv2; print(cv2.__version__)"`
4. CMake-Version prüfen: `cmake --version`

**Das System läuft nur mit korrekten lokalen Pfaden!**

## Erwartetes Verhalten

### **Raylib UI-Fenster (1000x700px)**:
- **2x2 Grid Layout** für 4 Fahrzeuge
- **Auto-1 (Oben Links)**: Gelb → Rot
- **Auto-2 (Oben Rechts)**: Gelb → Blau  
- **Auto-3 (Unten Links)**: Gelb → Grün
- **Auto-4 (Unten Rechts)**: Gelb → Lila
- **Status pro Panel**: Position, Winkel, Distanz, Kompass
- **Vereinfachte Darstellung**: Nur Hauptposition statt Front+Heck

### **OpenCV-Kamerafenster**:
- **Titel**: "Multi-Vehicle Detection - Vereinfachte Darstellung"
- **Fahrzeugdarstellung**: Großer farbiger Kreis an Hauptposition
- **Richtungspfeil**: Zeigt Fahrtrichtung  
- **Distanz-Anzeige**: Fahrzeuggröße in Pixeln
- **Status-Info**: "Erkannt: X/4 Autos" oben links
- **ESC-Taste**: Programm beenden

### **Vereinfachtes Datenmodell**:
```cpp
struct VehicleDetectionData {
    Point2D position;        // Hauptposition (Schwerpunkt)
    bool detected;           // Einfacher Status
    float angle;             // Richtung in Grad
    float distance;          // Abstand Front-Heck (Fahrzeuggröße)
    string rear_color;       // Identifikation (rot, blau, grün, lila)
}
```

## Troubleshooting

### Python nicht gefunden
```
Error: Python3 not found
```
**Lösung**: Python in PATH hinzufügen oder neu installieren

### OpenCV Fehler
```
ModuleNotFoundError: No module named 'cv2'
```
**Lösung**: `pip install opencv-python`

### Kamera nicht verfügbar
```
Multi-Vehicle Kamera initialisiert
Kein Kamerabild empfangen!
```
**Lösung**: 
- USB-Kamera anschließen und Treiber prüfen
- Andere Programme schließen, die die Kamera verwenden
- Kamera-Index in `MultiVehicleKamera.py` ändern (`cv2.VideoCapture(1)` statt `(0)`)

### Fahrzeuge werden nicht erkannt
**Symptome**: Raylib zeigt "NICHT ERKANNT" für alle Fahrzeuge
**Lösung**:
- Beleuchtung verbessern
- HSV-Werte in `MultiVehicleKamera.py` anpassen
- Fahrzeuge mit deutlichen gelben/farbigen Markierungen verwenden
- Kamera näher an die Fahrzeuge positionieren

### Build Fehler mit Python Libraries
**Lösung**: CMakeLists.txt prüfen - automatische Python-Erkennung sollte funktionieren

## System-Kompatibilität

### ✅ **Vereinfachte Architektur**:
- **Header-Only Design**: `Vehicle.h` ohne `.cpp` (nur Datenstrukturen)
- **Intelligente Zuordnung**: Distance-Based Assignment Algorithmus
- **Einheitliche Frontfarbe**: Alle Fahrzeuge haben gelbe Fronten
- **4 Heckfarben**: Rot, Blau, Grün, Lila zur Identifikation
- **Hauptposition**: Schwerpunkt zwischen Front- und Heckpunkt
- **Distanz-Messung**: Abstand Front-Heck als Fahrzeuggröße

### ✅ **Automatisch erkannt**:
- Python Installation path
- Python Include directories  
- Python Libraries
- Projekt source directory (relativer Pfad)

### ⚠️ **Möglicherweise anzupassen**:
- Nur bei exotischen Python-Installationen
- Bei mehreren Python-Versionen parallel

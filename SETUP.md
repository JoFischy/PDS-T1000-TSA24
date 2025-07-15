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
- ✅ Raylib-Fenster öffnet sich
- ✅ 2x2 Grid mit 4 Fahrzeug-Panels
- ✅ Kamera-Feed wird angezeigt
- ✅ Konsole zeigt: "Python interpreter initialized for Multi-Vehicle Fleet"

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
- Live-Kamerabild in allen 4 Panels
- Erkannte Fahrzeuge mit Position und Winkel
- Kompass-Anzeigen für Fahrtrichtung
- Status-Informationen in der Konsole

## 🔧 Erweiterte Konfiguration

### Kamera-Index ändern (falls mehrere Kameras):
In `MultiVehicleKamera.py` - Zeile ~20:
```python
cap = cv2.VideoCapture(0)  # Ändere 0 zu 1, 2, etc.
```

### HSV-Werte anpassen (bei anderen Lichtverhältnissen):
In `MultiVehicleKamera.py` - Funktionen `find_yellow_points()`, `find_red_points()`, etc.

## 🆘 Support

Bei Problemen:
1. Prüfe alle 4 Pfad-Anpassungen
2. Stelle sicher, dass Python 3.13.x installiert ist
3. Vergewissere dich, dass OpenCV funktioniert: `python -c "import cv2; print(cv2.__version__)"`
4. CMake-Version prüfen: `cmake --version`

**Das System läuft nur mit korrekten lokalen Pfaden!**

## Erwartetes Verhalten

- **Raylib-Fenster**: Zeigt Koordinaten der erkannten roten Objekte
- **Kamera-Fenster**: Live-Kamerabild mit grünen Rahmen um rote Objekte
- **Keine roten Objekte**: Leeres Raylib-Fenster
- **ESC**: Programm beenden

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
Error: Could not open video
```
**Lösung**: Andere Programme schließen die die Kamera verwenden

### Build Fehler mit Python Libraries
**Lösung**: CMakeLists.txt prüfen - automatische Python-Erkennung sollte funktionieren

## System-Kompatibilität

✅ **Automatisch erkannt**:
- Python Installation path
- Python Include directories  
- Python Libraries
- Projekt source directory

❌ **Manuell anpassen falls nötig**: Nur bei sehr exotischen Python-Installationen

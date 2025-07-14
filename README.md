# PDS-T1000-TSA24 - Auto-Erkennung mit Richtungsbestimmung

🚗 **Auto-Erkennungs-System** das rote (Heck) und gelbe (Spitze) Objekte erkennt und daraus die Fahrtrichtung in Grad berechnet.

## 🎯 Was das System macht

- **🟡 Gelb = Spitze** des Autos (Vorderseite)
- **🔴 Rot = Heck** des Autos (Rückseite)  
- **📐 Richtung** in Grad berechnen: **0° = oben, 90° = rechts, 180° = unten, 270° = links**
- **📊 Live-Anzeige** in Raylib mit Kompass und Koordinaten
- **👁️ Kamera-Feed** mit Markierungen und Richtungslinie

## 🚀 Schnellstart für Kollegen

### Voraussetzungen
- **Python 3.13+** mit folgenden Packages:
  ```bash
  pip install opencv-python numpy pybind11
  ```
- **CMake 3.11+**
- **Visual Studio 2022** (mit C++ Entwicklungstools)

### Installation & Build

1. **Repository klonen:**
   ```bash
   git clone https://github.com/JoFischy/PDS-T1000-TSA24.git
   cd PDS-T1000-TSA24
   ```

2. **Python Dependencies installieren:**
   ```bash
   pip install opencv-python numpy pybind11
   ```

3. **Build-Ordner erstellen und konfigurieren:**
   ```bash
   mkdir build
   cd build
   cmake .. "-DCMAKE_POLICY_VERSION_MINIMUM=3.11"
   ```

4. **Projekt kompilieren:**
   ```bash
   cmake --build . --config Debug
   ```

5. **Programm starten:**
   ```bash
   cd Debug
   ./camera_detection.exe    # Windows
   ./camera_detection        # Linux/macOS
   ```

## 🎯 Features

### Auto-Erkennungs-System
- **🟡 Gelbe Objekte** = Spitze/Vorderseite des Autos
- **🔴 Rote Objekte** = Heck/Rückseite des Autos
- **📐 Richtungsberechnung** zwischen Heck und Spitze in Grad (0° = oben)
- **📏 Abstandsmessung** zwischen den beiden Punkten

### Dual-Display System
- **OpenCV-Fenster**: Live-Kamerabild mit farbigen Markierungen und Richtungslinie
- **Raylib-Fenster**: Kompass, Koordinaten, Winkel und Status-Übersicht

### Performance-Optimiert
- Persistenter Python-Interpreter (wird nur einmal geladen)
- Optimierte Kamera-Settings (640x480 @ 30 FPS)  
- HSV-Farbraum für präzise Farberkennung
- Morphologie-Filter gegen Bildrauschen

## 📁 Projektstruktur

```
├── src/
│   ├── Kamera.py              # Auto-Erkennungsmodul (Rot + Gelb)
│   ├── main.cpp               # Raylib Hauptprogramm  
│   ├── py_runner.cpp          # Python-C++ Bridge
│   ├── CameraDisplay.cpp      # Auto-Anzeige mit Kompass
│   └── ...
├── include/                   # Header-Dateien
├── CMakeLists.txt            # Build-Konfiguration
└── README.md                 # Diese Datei
```

## 🛠️ Entwicklung

### Python Dependencies installieren
```bash
pip install opencv-python numpy pybind11
```

### Bei Problemen mit Raylib CMake
Falls CMake-Fehler auftreten, verwende:
```bash
cmake .. "-DCMAKE_POLICY_VERSION_MINIMUM=3.11"
```

### Debugging
- **Python-Fehler**: Prüfe ob alle Dependencies installiert sind
- **Kamera-Fehler**: Stelle sicher, dass eine Webcam angeschlossen ist
- **Build-Fehler**: Prüfe Visual Studio 2022 Installation

## 📋 Erkennungsparameter

- **Farbbereich (HSV)**: 
  - Rot 1: H(0-10), S(100-255), V(100-255)
  - Rot 2: H(160-179), S(100-255), V(100-255)
- **Mindestfläche**: 200 Pixel
- **Kamera-Auflösung**: 640x480
- **Frame-Rate**: 30 FPS

## 🎮 Bedienung

- **ESC**: Programm beenden
- **Fenster schließen**: Automatisches Cleanup
- **Rote Objekte**: Werden automatisch erkannt und markiert

## 🤝 Contribution

1. Feature-Branch erstellen: `git checkout -b feature/neue-funktion`
2. Änderungen commiten: `git commit -am 'Neue Funktion hinzugefügt'`
3. Branch pushen: `git push origin feature/neue-funktion`
4. Pull Request erstellen

## 📞 Support

Bei Problemen erstelle ein Issue auf GitHub oder wende dich an das Entwicklungsteam.

---
**Developed by Team PDS-T1000-TSA24** 🚀

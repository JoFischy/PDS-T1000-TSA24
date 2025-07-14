# Kamera Integration mit Raylib - Optimierte Version

## Übersicht
Dieses Projekt integriert Python-basierte Kameraerkennung mit Raylib für die Echtzeitanzeige von roten Objektkoordinaten. Das System verwendet einen persistenten Python-Interpreter für optimale Performance und zeigt sowohl das Live-Kamerabild als auch detaillierte Koordinateninformationen.

## Systemanforderungen
- **Python 3.13.5** (installiert unter: `C:\Users\jonas\AppData\Local\Programs\Python\Python313\python.exe`)
- **OpenCV**: ✅ Bereits installiert (Version 4.12.0.88)
- **NumPy**: ✅ Bereits installiert (Version 2.2.6)
- **pybind11**: ✅ Bereits installiert (Version 3.0.0)

## Dateistruktur
```
src/
├── Kamera.py              # Python-Kameramodul (im src Ordner)
├── main.cpp               # Raylib Hauptprogramm
├── py_runner.cpp          # Python-C++ Bridge mit persistentem Interpreter
├── CameraDisplay.cpp      # Raylib Display-Klasse
└── ...
```

## Funktionsweise

### Python Seite (src/Kamera.py)
- `get_red_object_coordinates()`: Hauptfunktion die Koordinaten von roten Objekten zurückgibt
- `get_red_object_coordinates_with_display()`: Zeigt Live-Kamerabild mit grünen Kästen um rote Objekte
- `init_camera()`: Initialisiert die Kamera mit optimalen Einstellungen
- `get_test_coordinates()`: Liefert animierte Testkoordinaten wenn keine Kamera verfügbar
- `cleanup_camera()`: Gibt Kamera-Ressourcen frei

### C++ Seite - Optimierter Interpreter
- **`initialize_python()`**: Initialisiert den Python-Interpreter einmal beim Start
- **Persistenter Interpreter**: Vermeidet wiederholte Interpreter-Starts für bessere Performance
- **Modul-Caching**: Lädt das Kamera-Modul einmal und wiederverwendet es
- `get_camera_coordinates_with_display()`: Ruft Python-Funktion mit Display auf
- `handle_opencv_events()`: Behandelt OpenCV-Fenster-Events
- `cleanup_camera()`: Räumt Kamera-Ressourcen auf

## Performance-Optimierungen

### Persistenter Python-Interpreter
- **Einmalige Initialisierung**: Python wird nur einmal beim Programmstart geladen
- **Modul-Wiederverwendung**: Kamera-Modul wird gecacht und wiederverwendet
- **Bessere Frame-Rate**: Eliminiert Interpreter-Startup-Zeit bei jedem Frame

### Kamera-Optimierungen  
- **640x480 Auflösung**: Optimaler Kompromiss zwischen Qualität und Performance
- **30 FPS**: Flüssige Bewegungen
- **Kamera Warm-up**: 5 Frames zum Aufwärmen der Kamera

## Features

### Live-Updates mit hoher Performance
- **Kontinuierliche Koordinaten-Updates**: 30 FPS ohne Verzögerung
- **Dual-Display**: Kamerabild (OpenCV) + Koordinaten-Info (Raylib)
- **Echtzeit-Erkennung**: Sofortige Objekterkennung und -markierung

### Visuelle Darstellung
- **Kamerafenster**: Live-Kamerabild mit grünen Kästen um erkannte rote Objekte
- **Koordinaten-Overlay**: Position direkt im Kamerabild angezeigt
- **Raylib-Fenster**: 
  - Live-Zeitstempel
  - Objektanzahl und Status
  - Detaillierte Koordinaten (x, y, Breite, Höhe)
  - Flächenberechnung
  - Skalierte Vorschau-Rechtecke

### Robustes Fallback-System
- Automatischer Wechsel zu animierten Testkoordinaten bei Kamera-Problemen
- Graceful Error Handling
- Automatisches Cleanup beim Beenden

## Build und Ausführung
```bash
cd build
cmake .. "-DCMAKE_POLICY_VERSION_MINIMUM=3.11"
cmake --build . --config Debug
.\Debug\raylib_example.exe
```

## Erkennungsparameter
- **HSV-Farbbereich für Rot**: 
  - Bereich 1: H(0-10), S(100-255), V(100-255)
  - Bereich 2: H(160-179), S(100-255), V(100-255)
- **Mindestfläche**: 200 Pixel (filtert kleine Störungen aus)
- **Update-Rate**: 30 FPS
- **Kamera-Auflösung**: 640x480 für optimale Performance

## Dependencies (Alle bereits installiert)
- OpenCV für Python (opencv-python 4.12.0.88)
- NumPy (2.2.6)  
- pybind11 (3.0.0)
- Raylib
- Python 3.13.5

## Keine .venv mehr benötigt!
Das System verwendet jetzt die globale Python-Installation, was die Verwaltung vereinfacht und die Performance verbessert.

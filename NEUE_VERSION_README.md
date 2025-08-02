# PDS-T1000-TSA24 - Vereinfachte Koordinaten-Erkennung mit Quadratischem Spielfeld

## Überblick
Vereinfachtes System zur Farberkennung und Koordinaten-Anzeige mit quadratischem Spielfeld-Raster.

## System-Komponenten

### 🎯 Quadratisches Koordinaten-System
- **Spielfeld-Raster**: Gesamtes Fenster wird in 40x40 Pixel Quadrate unterteilt
- **Maximiertes Fenster**: Nutzt fast den gesamten Bildschirm (minus Titelleiste/Taskleiste)
- **Konsistente Koordinaten**: Kamera-Koordinaten werden auf feste Spielfeld-Matrix gemappt
- **Skalierung**: Automatische Anpassung an Fenster-/Bildschirmgröße

### 🎨 Farberkennung (Farberkennung.py)
- **HSV-basierte Erkennung**: 5 verschiedene Farben (Front, Heck1-4)
- **Dynamische Crop-Bereiche**: Automatische Kamera-Bereich-Anpassung
- **Koordinaten-Normalisierung**: Liefert normalisierte Koordinaten für Spielfeld-Mapping

### 🖥️ Raylib-Anzeige (main.cpp)
- **Quadratisches Raster**: Sichtbares Gitter für Spielfeld-Orientierung
- **Farbcodierte Punkte**: Jede erkannte Farbe hat eigene Darstellung
- **Dual-Koordinaten**: Zeigt sowohl Spielfeld- (F) als auch Kamera-Koordinaten (C)
- **Responsive UI**: Punkte und Text skalieren mit Spielfeld-Größe

## Dokumentation

### 📋 Haupt-Dokumentation
- **[KOORDINATEN_SYSTEM_DOKUMENTATION.md](KOORDINATEN_SYSTEM_DOKUMENTATION.md)** - Ausführliche Erklärung des Koordinatensystems, Transformationen und Spielfeld-Zugriff

### 🔧 Schnell-Referenz
- **Spielfeld-Felder**: 10x10 Pixel, quadratisch
- **Koordinaten-Format**: `[Spalte,Zeile]` für Spielfeld, `(x,y)` für Kamera
- **Transformation**: Kamera → Crop → Normalisiert → Spielfeld → Fenster
- **Snap-to-Grid**: Alle Punkte landen automatisch in Feldmitte

## Struktur

```
├── src/
│   ├── main.cpp           # Hauptprogramm: Quadratisches Spielfeld
│   ├── py_runner.cpp      # Python-C++ Bridge
│   └── Farberkennung.py   # HSV-Farberkennung
├── include/
│   ├── Vehicle.h          # Datenstrukturen (DetectedObject)
│   ├── py_runner.h        # Python-Interface
│   └── pybind11/          # Python-Binding Headers
└── external/raylib/       # Raylib Bibliothek
```

## Koordinaten-Transformation

### 1. Kamera → Normalisiert (0.0 - 1.0)
```cpp
norm_x = camera_x / crop_width
norm_y = camera_y / crop_height
```

### 2. Normalisiert → Spielfeld-Pixel
```cpp
field_x = norm_x * field_width
field_y = norm_y * field_height
```

### 3. Spielfeld → Fenster-Pixel
```cpp
window_x = field_offset_x + field_x
window_y = field_offset_y + field_y
```

## Technische Spezifikationen

### Spielfeld-Parameter
```cpp
const int FIELD_SIZE = 40;    // Quadratische Felder: 40x40 Pixel
const int UI_HEIGHT = 80;     // Platz für Status-UI oben
```

### Erkannte Farben
- **Front**: ORANGE (vorne)
- **Heck1**: BLUE (hinten)
- **Heck2**: GREEN (hinten)
- **Heck3**: YELLOW (hinten)
- **Heck4**: PURPLE (hinten)

## Kompilierung
```bash
g++ -std=c++17 -Wall -Wextra -Iexternal/raylib/src -Iinclude -I"C:/Program Files/Python311/include" src/main.cpp src/py_runner.cpp -Lexternal/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -L"C:/Program Files/Python311/libs" -lpython311 -o main
```

## Verwendung

### Mit VS Code (F5)
Das Projekt ist für F5-Debugging konfiguriert und startet automatisch beide Fenster:
- Kamera-Fenster für Farberkennung
- Raylib-Fenster mit quadratischem Spielfeld

### Manuell
```bash
./main.exe
```

## Steuerung
- **ESC**: Programm beenden
- **F11**: Vollbild ein/aus
- **Maus**: Fenster verschieben und Größe ändern

## Ausgabe-Beispiel
```
=== KOORDINATEN-ANZEIGE MIT QUADRATISCHEM SPIELFELD ===
Maximiertes Fenster 1820x930 mit erkannten Koordinaten
Monitor: 1920x1080
Quadratische Felder: 40x40 Pixel pro Feld
ESC = Beenden | F11 = Vollbild umschalten | Fenster verschiebbar
=======================================================
Objekte: 2 | Spielfeld: 45x21 Felder | Auflösung: 1820x930
```

## Architektur-Vorteile
1. **Konsistente Koordinaten**: Unabhängig von Fenstergröße
2. **Spielfeld-Integration**: Vorbereitet für Spiel-Logik
3. **Quadratische Felder**: Perfekt für rasterbasierte Spiele
4. **Skalierbar**: Funktioniert auf verschiedenen Bildschirmgrößen
5. **Visuell**: Raster macht Koordinaten-System sichtbar

## Koordinaten

Die Farberkennung liefert normalisierte Koordinaten relativ zum Crop-Bereich:
- X: 0 bis crop_width
- Y: 0 bis crop_height
- Ursprung (0,0) = oben links

## Entfernte Komponenten

- BeamerProjection (komplexe Projektion)
- MultiCarDisplay (Multi-Fahrzeug-Anzeige)
- VehicleFleet (Fahrzeugverwaltung)
- MultiVehicleKamera.py (alte Erkennung)
- Alle anderen Python-Erkennungsskripte

Das System ist jetzt deutlich einfacher und fokussiert sich nur auf die Koordinaten-Übertragung von Python zu C++.

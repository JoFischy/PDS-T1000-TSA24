# Vereinfachtes PDS-T1000-TSA24 Projekt

## Überblick

Dieses Projekt wurde vereinfacht und enthält jetzt nur noch:

1. **Ein weißes Raylib-Fenster** (`src/main.cpp`) das erkannte Koordinaten anzeigt
2. **Eine neue Farberkennung** (`Farberkennung.py`) die Objekte in verschiedenen Farben erkennt
3. **Python-C++ Integration** über pybind11 (`src/py_runner.cpp`)

## Struktur

```
├── src/
│   ├── main.cpp           # Hauptprogramm: Weißes Raylib-Fenster
│   └── py_runner.cpp      # Python-C++ Bridge
├── include/
│   ├── Vehicle.h          # Datenstrukturen
│   ├── py_runner.h        # Python-Interface
│   └── pybind11/          # Python-Binding Headers
├── Farberkennung.py       # Neue Koordinaten-Erkennung
└── external/raylib/       # Raylib Bibliothek
```

## Kompilierung

```bash
g++ -std=c++17 -Wall -Wextra -Iexternal/raylib/src -Iinclude -I"C:/Program Files/Python311/include" src/main.cpp src/py_runner.cpp -Lexternal/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -L"C:/Program Files/Python311/libs" -lpython311 -o main
```

## Verwendung

1. **Farberkennung allein testen:**
   ```bash
   python Farberkennung.py
   ```

2. **C++ Programm starten:**
   ```bash
   ./main.exe
   ```

## Erkannte Farben

- `Front`: Orange Markierung (vorne)
- `Heck1`: Blaue Markierung (hinten)
- `Heck2`: Grüne Markierung (hinten) 
- `Heck3`: Gelbe Markierung (hinten)
- `Heck4`: Lila/Magenta Markierung (hinten)

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

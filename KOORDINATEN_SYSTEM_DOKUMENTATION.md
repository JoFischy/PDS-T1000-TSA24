# Koordinaten-System Dokumentation

## Überblick
Das PDS-T1000-TSA24 System verwendet ein mehrstufiges Koordinaten-Transformations-System, das Kamera-Koordinaten in ein quadratisches Spielfeld-Raster umwandelt.

---

## 🎯 Koordinaten-Transformation Pipeline

### 1. Kamera-Koordinaten (Pixel)
**Quelle**: `src/Farberkennung.py`
- **Ursprung**: Oben-links der Kamera
- **Einheit**: Pixel
- **Bereich**: 0 bis Kamera-Auflösung (z.B. 640x480)

```python
# Beispiel: Objekt bei Pixel (320, 240) in 640x480 Kamera
camera_x = 320  # Pixel von links
camera_y = 240  # Pixel von oben
```

### 2. Crop-Koordinaten (Pixel)
**Quelle**: `src/Farberkennung.py` - `crop_frame()`
- **Ursprung**: Oben-links des Crop-Bereichs
- **Einheit**: Pixel
- **Bereich**: 0 bis Crop-Größe (z.B. 540x380)

```python
# Nach Crop-Anwendung (50px Rand entfernt)
crop_x = camera_x - crop_left   # 320 - 50 = 270
crop_y = camera_y - crop_top    # 240 - 50 = 190
crop_width = 540   # 640 - 50 - 50
crop_height = 380  # 480 - 50 - 50
```

### 3. Normalisierte Koordinaten (0.0 - 1.0)
**Quelle**: `src/Farberkennung.py` - `normalize_coordinates()`
- **Ursprung**: Oben-links (0.0, 0.0)
- **Einheit**: Normalisiert
- **Bereich**: 0.0 bis 1.0

```python
# Normalisierung für plattformunabhängige Übertragung
norm_x = crop_x / crop_width    # 270 / 540 = 0.5
norm_y = crop_y / crop_height   # 190 / 380 = 0.5
```

### 4. Spielfeld-Koordinaten (Spalte/Zeile)
**Quelle**: `src/main.cpp` - `cameraToField()`
- **Ursprung**: Oben-links (0, 0)
- **Einheit**: Ganzzahl-Felder
- **Bereich**: 0 bis field_cols/field_rows

```cpp
// Umwandlung in Spielfeld-Raster (z.B. 182x93 Felder bei 1820x930 Fenster)
field_col = (int)(norm_x * field_cols)  // 0.5 * 182 = 91
field_row = (int)(norm_y * field_rows)  // 0.5 * 93 = 46
```

### 5. Fenster-Koordinaten (Pixel)
**Quelle**: `src/main.cpp` - `fieldToWindow()`
- **Ursprung**: Oben-links des Fensters
- **Einheit**: Pixel
- **Bereich**: 0 bis Fenster-Größe

```cpp
// Zentrierung in Feldmitte + Offset für UI
window_x = offset_x + (field_col + 0.5f) * FIELD_SIZE  // 50 + (91 + 0.5) * 10 = 965
window_y = offset_y + (field_row + 0.5f) * FIELD_SIZE  // 130 + (46 + 0.5) * 10 = 595
```

---

## 🔧 Spielfeld-Raster Spezifikationen

### Feld-Parameter
```cpp
const int FIELD_SIZE = 10;    // 10x10 Pixel pro Feld
const int UI_HEIGHT = 80;     // Platz für Status-UI oben
```

### Raster-Berechnung
```cpp
// Verfügbare Fläche
available_width = window_width;                    // z.B. 1820px
available_height = window_height - UI_HEIGHT;     // z.B. 930 - 80 = 850px

// Anzahl Felder
field_cols = available_width / FIELD_SIZE;        // 1820 / 10 = 182 Spalten
field_rows = available_height / FIELD_SIZE;       // 850 / 10 = 85 Zeilen

// Tatsächliche Spielfeld-Größe
field_width = field_cols * FIELD_SIZE;            // 182 * 10 = 1820px
field_height = field_rows * FIELD_SIZE;           // 85 * 10 = 850px

// Zentrierung (falls Fenster nicht perfekt teilbar)
offset_x = (window_width - field_width) / 2;      // (1820 - 1820) / 2 = 0
offset_y = UI_HEIGHT + (available_height - field_height) / 2;  // 80 + (850 - 850) / 2 = 80
```

---

## 📐 Koordinaten-Zugriff und Manipulation

### Python → C++ Datenübertragung
**Datei**: `src/py_runner.cpp`

```cpp
// DetectedObject Struktur
struct DetectedObject {
    Point2D coordinates;        // Ursprüngliche Pixel-Koordinaten im Crop
    std::string color;          // "Front", "Heck1", "Heck2", "Heck3", "Heck4"
    float crop_width;           // Breite des Crop-Bereichs
    float crop_height;          // Höhe des Crop-Bereichs
};
```

### Spielfeld-Zugriff
```cpp
// Direkte Feld-Adressierung
int getFieldAtPosition(int col, int row) {
    if (col >= 0 && col < field_cols && row >= 0 && row < field_rows) {
        return row * field_cols + col;  // Eindeutige Feld-ID
    }
    return -1;  // Ungültige Position
}

// Position zu Feld-ID
int positionToFieldID(float window_x, float window_y) {
    int col = (int)((window_x - offset_x) / FIELD_SIZE);
    int row = (int)((window_y - offset_y) / FIELD_SIZE);
    return getFieldAtPosition(col, row);
}
```

---

## 🎮 Spielfeld-Integration Beispiele

### 1. Objekt-Platzierung
```cpp
// Erkanntes Objekt auf Spielfeld platzieren
void placeObjectOnField(const DetectedObject& obj) {
    int field_col, field_row;
    cameraToField(obj, field_transform, field_col, field_row);
    
    // Spielfeld-Array aktualisieren
    if (field_col >= 0 && field_col < field_cols && 
        field_row >= 0 && field_row < field_rows) {
        
        int field_id = field_row * field_cols + field_col;
        game_field[field_id] = obj.color;  // z.B. "Front", "Heck1"
        
        printf("Objekt %s platziert bei Feld [%d,%d] (ID: %d)\n", 
               obj.color.c_str(), field_col, field_row, field_id);
    }
}
```

### 2. Kollisions-Erkennung
```cpp
// Prüfe ob zwei Objekte im selben Feld sind
bool checkCollision(int col1, int row1, int col2, int row2) {
    return (col1 == col2 && row1 == row2);
}

// Prüfe Nachbarfelder
bool isAdjacent(int col1, int row1, int col2, int row2) {
    int dx = abs(col1 - col2);
    int dy = abs(row1 - row2);
    return (dx <= 1 && dy <= 1 && !(dx == 0 && dy == 0));
}
```

### 3. Pfadfindung
```cpp
// Manhatttan-Distanz zwischen zwei Feldern
int manhattanDistance(int col1, int row1, int col2, int row2) {
    return abs(col1 - col2) + abs(row1 - row2);
}

// Direkte Linie prüfen
bool hasDirectPath(int start_col, int start_row, int end_col, int end_row) {
    // Implementiere Bresenham-Algorithmus für Linienverfolgung
    // Prüfe ob alle Zwischenfelder frei sind
}
```

---

## 📊 Debug-Informationen

### Koordinaten-Ausgabe Format
```
Front [91,46] C(270,190)
│     │  │    │   │   │
│     │  │    │   │   └── Original Crop Y-Koordinate
│     │  │    │   └────── Original Crop X-Koordinate  
│     │  │    └────────── "C" = Camera/Crop Koordinaten
│     │  └─────────────── Spielfeld Zeile (Row)
│     └────────────────── Spielfeld Spalte (Column)
└──────────────────────── Erkannte Farbe
```

### Status-Informationen
```
Objekte: 3 | Spielfeld: 182x85 Felder | Auflösung: 1820x850
│        │   │            │      │       │          │    │
│        │   │            │      │       │          │    └── Fenster Höhe
│        │   │            │      │       │          └─────── Fenster Breite
│        │   │            │      │       └────────────────── "Auflösung" Label
│        │   │            │      └──────────────────────── Spielfeld Zeilen
│        │   │            └─────────────────────────────── Spielfeld Spalten
│        │   └──────────────────────────────────────────── "Spielfeld" Label
│        └────────────────────────────────────────────── Anzahl erkannte Objekte
└─────────────────────────────────────────────────────── "Objekte" Label
```

---

## 🔄 Koordinaten-Umrechnungs-Funktionen

### Vollständige Transformation Chain
```cpp
// Von Kamera zu Spielfeld (komplette Pipeline)
void cameraToFieldComplete(float camera_x, float camera_y, 
                          float crop_left, float crop_top,
                          float crop_width, float crop_height,
                          int& field_col, int& field_row) {
    
    // 1. Kamera → Crop
    float crop_x = camera_x - crop_left;
    float crop_y = camera_y - crop_top;
    
    // 2. Crop → Normalisiert
    float norm_x = crop_x / crop_width;
    float norm_y = crop_y / crop_height;
    
    // 3. Normalisiert → Spielfeld
    field_col = (int)(norm_x * field_cols);
    field_row = (int)(norm_y * field_rows);
    
    // 4. Bounds checking
    field_col = std::max(0, std::min(field_col, field_cols - 1));
    field_row = std::max(0, std::min(field_row, field_rows - 1));
}
```

### Rück-Transformation (Spielfeld zu Fenster)
```cpp
// Von Spielfeld zu Fenster-Pixel
void fieldToWindowComplete(int field_col, int field_row, 
                          float& window_x, float& window_y) {
    
    // Zentriere in Feldmitte
    window_x = offset_x + (field_col + 0.5f) * FIELD_SIZE;
    window_y = offset_y + (field_row + 0.5f) * FIELD_SIZE;
}
```

---

## ⚡ Performance-Optimierungen

### Lookup-Tabellen
```cpp
// Pre-berechnete Fenster-Positionen für jedes Feld
std::vector<std::pair<float, float>> field_window_positions;

void precomputeFieldPositions() {
    field_window_positions.resize(field_cols * field_rows);
    
    for (int row = 0; row < field_rows; row++) {
        for (int col = 0; col < field_cols; col++) {
            int index = row * field_cols + col;
            field_window_positions[index] = {
                offset_x + (col + 0.5f) * FIELD_SIZE,
                offset_y + (row + 0.5f) * FIELD_SIZE
            };
        }
    }
}
```

### Schnelle Feld-Abfragen
```cpp
// O(1) Zugriff auf Feld-Eigenschaften
std::vector<std::string> field_occupancy;  // field_cols * field_rows Elemente

void updateFieldOccupancy(int col, int row, const std::string& color) {
    int index = row * field_cols + col;
    if (index >= 0 && index < field_occupancy.size()) {
        field_occupancy[index] = color;
    }
}

std::string getFieldOccupancy(int col, int row) {
    int index = row * field_cols + col;
    if (index >= 0 && index < field_occupancy.size()) {
        return field_occupancy[index];
    }
    return "";  // Leeres Feld
}
```

---

## 🎯 Praktische Anwendungsbeispiele

### Spiel-Logik Integration
```cpp
// Beispiel: Einfaches Fang-Spiel
class SimpleGame {
    std::vector<DetectedObject> current_objects;
    
public:
    void updateGame(const std::vector<DetectedObject>& detected) {
        current_objects = detected;
        
        // Finde Front-Objekt
        int front_col = -1, front_row = -1;
        for (const auto& obj : detected) {
            if (obj.color == "Front") {
                cameraToField(obj, field_transform, front_col, front_row);
                break;
            }
        }
        
        // Prüfe Kollisionen mit Heck-Objekten
        for (const auto& obj : detected) {
            if (obj.color.find("Heck") == 0) {  // Startet mit "Heck"
                int heck_col, heck_row;
                cameraToField(obj, field_transform, heck_col, heck_row);
                
                if (checkCollision(front_col, front_row, heck_col, heck_row)) {
                    printf("Kollision! Front trifft %s bei [%d,%d]\n", 
                           obj.color.c_str(), heck_col, heck_row);
                }
            }
        }
    }
};
```

Dieses Koordinatensystem bietet eine solide Grundlage für komplexe Spielfeld-basierte Anwendungen mit präziser Positionierung und effizienter Verarbeitung! 🎯

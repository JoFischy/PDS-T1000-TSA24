# 🚗 Koordinaten- und Richtungs-Schnittstellen Dokumentation

## 📋 Übersicht
Diese Dokumentation beschreibt die komplette Datenstruktur und Koordinatenverarbeitung für die Fahrzeugerkennung im PDS-T1000-TSA24 Projekt. Das System verwendet eine Python-Computer-Vision-Pipeline mit C++/Raylib-Frontend.

## 🏗️ Systemarchitektur

```
Python (OpenCV/Computer Vision) → C++ Bridge → Raylib (GUI/Beamer)
     MultiVehicleKamera.py      →  py_runner.cpp  →  BeamerProjection.cpp
```

## 📊 Haupt-Datenstrukturen

### 1. C++ Hauptstruktur (`include/Vehicle.h`)

```cpp
// 2D Punkt für Koordinaten
struct Point2D {
    float x, y;
    Point2D() : x(0), y(0) {}
    Point2D(float x_, float y_) : x(x_), y(y_) {}
};

// Hauptdatenstruktur für Fahrzeugerkennung
struct VehicleDetectionData {
    // Hauptposition (Schwerpunkt zwischen Front und Heck)
    Point2D position;           // Hauptkoordinate des Fahrzeugs
    
    // Erkennungsstatus
    bool detected;              // Fahrzeug vollständig erkannt?
    
    // Richtung und Größe
    float angle;                // Richtung in Grad (0° = nach oben)
    float distance;             // Abstand zwischen Front- und Heckpunkt (Fahrzeuggröße)
    
    // Identifikation
    std::string rear_color;     // Heckfarbe zur Identifikation (blau, grün, gelb, lila)
    
    // Standardkonstruktor
    VehicleDetectionData() : position(0, 0), detected(false), angle(0), distance(0), rear_color("") {}
};
```

### 2. Python Erkennungsdaten (`src/MultiVehicleKamera.py`)

```python
detection = {
    'vehicle_name': str,          # "Auto-1", "Auto-2", etc.
    'front_color': str,           # "Orange" (einheitlich für alle)
    'rear_color': str,            # "Blau", "Grün", "Gelb", "Lila"
    'front_pos': (float, float),  # Pixel-Koordinaten der vorderen Farbe
    'rear_pos': (float, float),   # Pixel-Koordinaten der hinteren Farbe
    'has_front': bool,            # Vordere Farbe erkannt?
    'has_rear': bool,             # Hintere Farbe erkannt?
    'has_angle': bool,            # Richtung berechenbar?
    'angle_degrees': float,       # Fahrtrichtung in Grad
    'distance_pixels': float      # Abstand zwischen Front/Heck in Pixeln
}
```

## 🧭 Koordinatensysteme und Transformationen

### 1. Kamera-Koordinatensystem (Python/OpenCV)
- **Ursprung**: Oben links (0,0)
- **X-Achse**: Nach rechts positiv
- **Y-Achse**: Nach unten positiv  
- **Einheit**: Pixel
- **Bereich**: 0 bis Kameraauflösung (z.B. 640x480)

### 2. Fahrzeug-Koordinatensystem (Logisch)
- **Position**: Schwerpunkt zwischen vorderer und hinterer Farbe
- **Berechnung**: `position = (front_pos + rear_pos) / 2`
- **X-Achse**: Rechts/Links Bewegung
- **Y-Achse**: Vor/Zurück Bewegung

### 3. Raylib-Koordinatensystem (Anzeige)
- **Ursprung**: Oben links (0,0)
- **X-Achse**: Nach rechts positiv
- **Y-Achse**: Nach unten positiv
- **Transformation**: 1:1 von Kamera-Koordinaten
- **Skalierung**: 1000x1000 logische Einheiten auf Bildschirmgröße

### 4. Koordinaten-Transformation (BeamerProjection.cpp)

```cpp
// Skalierung von Kamera-Pixel zu Bildschirm-Pixel (verschiebbares Fenster)
float scale_x = (screen_width - 2 * CAMERA_FRAME_WIDTH) / 1000.0f;
float scale_y = (screen_height - 2 * CAMERA_FRAME_WIDTH) / 1000.0f;

// Direkte Zuordnung - KEINE Y-Invertierung!
int x = (int)(vehicle.position.x * scale_x) + CAMERA_FRAME_WIDTH;
int y = (int)(vehicle.position.y * scale_y) + CAMERA_FRAME_WIDTH;
```

### 5. Kamera-Markierungsrahmen (Neu!)

```cpp
void BeamerProjection::draw_camera_frame() {
    Color camera_frame_color = PURPLE;  // Lila Rahmen für Kamera-Erkennung
    
    // 25 Pixel breiter Rahmen (ca. 0.5cm bei 96 DPI)
    DrawRectangle(0, 0, screen_width, CAMERA_FRAME_WIDTH, camera_frame_color);
    // ... weitere Rahmen-Seiten
}
```

## 🎯 Richtungs-/Winkelberechnung

### 1. Python Winkelberechnung (`MultiVehicleKamera.py`)

```python
def calculate_angle(self, front_pos, rear_pos):
    """
    Berechnet Fahrtrichtung basierend auf Front-/Heck-Position
    
    Returns:
        float: Winkel in Grad (0° = nach oben, 90° = nach rechts)
    """
    dx = front_pos[0] - rear_pos[0]  # X-Differenz
    dy = front_pos[1] - rear_pos[1]  # Y-Differenz
    
    # atan2(dx, -dy) für 0° = nach oben
    angle_rad = math.atan2(dx, -dy)
    angle_deg = math.degrees(angle_rad)
    
    # Normalisiere zu 0-360°
    if angle_deg < 0:
        angle_deg += 360
        
    return angle_deg
```

### 2. C++ Winkel-Rendering (`BeamerProjection.cpp`)

```cpp
// Winkel-Korrektur für Raylib-Koordinatensystem
// Python: 0° = nach oben, 90° = nach rechts
// Raylib: 0° = nach rechts, 90° = nach unten
float corrected_angle = (vehicle.angle - 90.0f) * DEG2RAD;

// Richtungspfeil zeichnen
int arrow_x = x + (int)(cos(corrected_angle) * arrow_length);
int arrow_y = y + (int)(sin(corrected_angle) * arrow_length);
DrawLine(x, y, arrow_x, arrow_y, BLACK);
```

## 🔗 Python-C++ Bridge (`src/py_runner.cpp`)

### Datenübertragung von Python zu C++

```cpp
std::vector<VehicleDetectionData> get_all_vehicle_detections() {
    // Rufe Python-Funktion auf
    py::object detections = multi_vehicle_module.attr("get_multi_vehicle_detections")();
    py::list detection_list = detections.cast<py::list>();
    
    for (auto item : detection_list) {
        py::dict detection = item.cast<py::dict>();
        VehicleDetectionData vehicle_data;
        
        // Position = Mittelpunkt zwischen Front und Heck
        py::tuple front_pos = detection["front_pos"].cast<py::tuple>();
        py::tuple rear_pos = detection["rear_pos"].cast<py::tuple>();
        
        float front_x = front_pos[0].cast<float>();
        float front_y = front_pos[1].cast<float>();
        float rear_x = rear_pos[0].cast<float>();
        float rear_y = rear_pos[1].cast<float>();
        
        vehicle_data.position = Point2D((front_x + rear_x) / 2.0f, (front_y + rear_y) / 2.0f);
        
        // Status und weitere Daten
        vehicle_data.detected = detection["has_front"].cast<bool>() && detection["has_rear"].cast<bool>();
        vehicle_data.angle = detection["angle_degrees"].cast<float>();
        vehicle_data.distance = detection["distance_pixels"].cast<float>();
        vehicle_data.rear_color = detection["rear_color"].cast<std::string>();
        
        results.push_back(vehicle_data);
    }
    return results;
}
```

## 🎨 Fahrzeug-Identifikation

### Farbkonfiguration (4 Fahrzeuge)
```cpp
// Alle Fahrzeuge haben einheitliche ORANGE Vorderseite
// Unterscheidung durch Heckfarbe:
global_fleet->add_vehicle("Auto-1", "Orange", "Blau");   // Auto-1: Blau hinten
global_fleet->add_vehicle("Auto-2", "Orange", "Grün");   // Auto-2: Grün hinten  
global_fleet->add_vehicle("Auto-3", "Orange", "Gelb");   // Auto-3: Gelb hinten
global_fleet->add_vehicle("Auto-4", "Orange", "Lila");   // Auto-4: Lila hinten
```

### Intelligente Paar-Zuordnung
1. **Erkennung**: Alle orangenen Punkte (Vorderseite) und alle Identifikator-Farben (Heckseite)
2. **Zuordnung**: Nächster-Nachbar-Algorithmus mit maximal 200 Pixel Abstand
3. **Vermeidung**: Verhindert Doppel-Zuordnungen durch Used-Set

## 📱 Raylib GUI-Komponenten

### 1. MultiCarDisplay (Detailansicht)
- **Layout**: 2x2 Grid für 4 Fahrzeuge
- **Anzeige**: Kompass, Koordinaten, Status, Fahrzeugfarbe
- **Update**: `vehicle_data = get_all_vehicle_detections()`

### 2. BeamerProjection (Overhead-Projektion)
- **Darstellung**: Farbige Rechtecke (20x30 Pixel) mit Richtungspfeil
- **Farbzuordnung**: Fahrzeugfarbe = Heckfarbe (Blau, Grün, Gelb, Lila)
- **Koordinaten**: Direkte Kamera→Bildschirm Transformation
- **Features**: Schwarzer Rand, Warnmodus, Fahrzeug-IDs

## 🔄 Hauptprogramm-Loop (`src/main.cpp`)

```cpp
while (!WindowShouldClose()) {
    // 1. Hole aktuelle Fahrzeugdaten von Python
    std::vector<VehicleDetectionData> vehicle_data = get_all_vehicle_detections();
    
    // 2. Update Beamer-Projektion
    beamer.update(vehicle_data);
    
    // 3. Zeige OpenCV Debug-Fenster (parallel)
    show_fleet_camera_feed();
    
    // 4. Verarbeite OpenCV Events
    handle_opencv_events();
    
    // 5. Render Raylib-Beamer-Ausgabe
    beamer.draw();
}
```

## ⚙️ Wichtige Konfigurationsparameter

### Computer Vision (MultiVehicleKamera.py)
```python
MIN_FRONT_AREA = 100     # Mindestgröße für Orange-Erkennung
MIN_REAR_AREA = 80       # Mindestgröße für Heck-Farben
MAX_PAIR_DISTANCE = 200  # Max Pixel-Abstand für Paar-Zuordnung
```

### Raylib Rendering (BeamerProjection.cpp)
```cpp
const int CAMERA_FRAME_WIDTH = 25;       // Lila Kamera-Markierungsrahmen (ca. 0.5cm)
const int VEHICLE_WIDTH = 20;            // Fahrzeug-Rechteck Breite
const int VEHICLE_HEIGHT = 30;           // Fahrzeug-Rechteck Höhe

// Großes verschiebbares Fenster für Beamer-Projektion
int window_width = monitor_width - 100;   // Fast Vollbild, aber verschiebbar
int window_height = monitor_height - 100;
SetWindowState(FLAG_WINDOW_RESIZABLE);    // Fenster kann vergrößert werden
```

## 🚨 Wichtige Hinweise für Raylib-Design

1. **Verschiebbares Fenster**: Das Raylib-Fenster ist fast vollbild, aber verschiebbar und vergrößerbar
2. **Lila Kamera-Rahmen**: 25 Pixel breiter Rahmen als Referenz für Kamera-Erkennung
3. **Koordinaten-Konsistenz**: Keine Y-Invertierung zwischen Kamera und Raylib!
4. **Winkel-Transformation**: Python-Winkel um 90° korrigieren für Raylib
5. **Skalierung**: Kamera-Erkennungsbereich (ohne Rahmen) wird auf 1000x1000 logische Einheiten skaliert
6. **Threading**: OpenCV läuft parallel zu Raylib (handle_opencv_events() erforderlich)
7. **Farb-Mapping**: Heckfarbe bestimmt Fahrzeugdarstellung in Raylib
8. **Position**: `vehicle.position` ist bereits der Schwerpunkt, nicht Front/Heck einzeln
9. **Bedienung**: ESC = Beenden, F11 = Vollbild umschalten, Fenster verschiebbar
10. **Flexibilität**: Fenster kann manuell auf gewünschte Beamer-Größe angepasst werden

## 📞 Schnittstellen-Funktionen für Raylib-Entwicklung

### Hauptfunktionen (`include/py_runner.h`)
```cpp
// Fahrzeugflotte initialisieren
bool initialize_vehicle_fleet();

// Alle Fahrzeugdaten holen (Hauptfunktion!)
std::vector<VehicleDetectionData> get_all_vehicle_detections();

// Kamera-Feed anzeigen (Debug)
void show_fleet_camera_feed();

// OpenCV Events verarbeiten (wichtig!)
void handle_opencv_events();

// Cleanup
void cleanup_vehicle_fleet();
```

### Beamer-Projektion (`include/BeamerProjection.h`)
```cpp
// Fahrzeugdaten update
void update(const std::vector<VehicleDetectionData>& fleet_data);

// Hauptrender-Funktion
void draw();

// Warnmodus umschalten
void set_warning_mode(bool enabled);
```

---
*Diese Dokumentation wurde erstellt für die Raylib-GUI-Entwicklung im PDS-T1000-TSA24 Projekt.*

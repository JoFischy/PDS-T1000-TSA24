# 📷 Kamera-Integration Dokumentation

## 🎯 Überblick

Das Multi-Vehicle Fleet System verwendet eine intelligente Computer Vision Pipeline zur Erkennung von 4 Fahrzeugen mit einheitlicher gelber Frontfarbe und individuellen Heckfarben.

## 🏗️ Architektur

### System-Komponenten
```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Kamera Feed   │───▶│  Python OpenCV   │───▶│  C++ Raylib UI  │
│   (USB Kamera)  │    │  Vision Engine   │    │  (2x2 Display)  │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                              │
                       ┌──────▼──────┐
                       │  pybind11   │
                       │   Bridge    │
                       └─────────────┘
```

## 🔍 Computer Vision Pipeline

### 1. Bilderfassung (`MultiVehicleKamera.py`)
```python
cap = cv2.VideoCapture(0)  # USB-Kamera (Index 0)
ret, frame = cap.read()    # Frame-by-Frame Erfassung
```

### 2. HSV-Farbfilterung
Das System arbeitet im HSV-Farbraum für robuste Farberkennung:

#### Gelb (Einheitliche Frontfarbe)
```python
# HSV-Werte für Gelb
lower_yellow = np.array([20, 100, 100])
upper_yellow = np.array([30, 255, 255])
mask_yellow = cv2.inRange(hsv, lower_yellow, upper_yellow)
```

#### Heckfarben (4 verschiedene)
```python
# Rot: H: 0-10 & 170-180 (Rot überspannt HSV-Grenze)
lower_red1 = np.array([0, 100, 100])
upper_red1 = np.array([10, 255, 255])
lower_red2 = np.array([170, 100, 100])
upper_red2 = np.array([180, 255, 255])

# Blau: H: 100-130
lower_blue = np.array([100, 100, 100])
upper_blue = np.array([130, 255, 255])

# Grün: H: 50-80
lower_green = np.array([50, 100, 100])
upper_green = np.array([80, 255, 255])

# Lila: H: 130-160
lower_purple = np.array([130, 100, 100])
upper_purple = np.array([160, 255, 255])
```

### 3. Morphologische Operationen
```python
# Rauschreduzierung
kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
```

### 4. Konturerkennung
```python
contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
for contour in contours:
    if cv2.contourArea(contour) > min_area:  # Mindestgröße
        M = cv2.moments(contour)
        if M["m00"] != 0:
            cx = int(M["m10"] / M["m00"])  # Schwerpunkt X
            cy = int(M["m01"] / M["m00"])  # Schwerpunkt Y
```

## 🧠 Intelligenter Pairing-Algorithmus

### Problem
- Mehrere gelbe Frontpunkte erkannt
- Mehrere verschiedenfarbige Heckpunkte erkannt  
- Welcher Frontpunkt gehört zu welchem Heckpunkt?

### Lösung: Distance-Based Assignment
```python
def assign_closest_pairs(front_points, rear_points):
    pairs = []
    used_rear = set()
    
    # Sortiere Frontpunkte nach X-Koordinate für konsistente Reihenfolge
    front_points.sort(key=lambda p: p[0])
    
    for front in front_points:
        closest_rear = None
        min_distance = float('inf')
        
        for rear_color, rear_pos in rear_points:
            if rear_pos in used_rear:
                continue
                
            distance = math.sqrt((front[0] - rear_pos[0])**2 + 
                               (front[1] - rear_pos[1])**2)
            
            if distance < min_distance and distance <= MAX_DISTANCE:
                min_distance = distance
                closest_rear = (rear_color, rear_pos)
        
        if closest_rear:
            used_rear.add(closest_rear[1])
            pairs.append((front, closest_rear))
    
    return pairs
```

### Parameter
- **MAX_DISTANCE**: 200 Pixel (maximaler Abstand zwischen Front- und Heckpunkt)
- **Kollisionsvermeidung**: Jeder Heckpunkt kann nur einmal zugeordnet werden

## 📐 Winkelberechnung

### Fahrtrichtung bestimmen
```python
def calculate_angle(front_point, rear_point):
    dx = front_point[0] - rear_point[0]
    dy = front_point[1] - rear_point[1]
    angle = math.atan2(dy, dx) * 180 / math.pi
    
    # Normalisierung auf 0-360°
    if angle < 0:
        angle += 360
    
    return angle
```

### Kompass-Darstellung
- **0°**: Nach rechts
- **90°**: Nach oben  
- **180°**: Nach links
- **270°**: Nach unten

## 🔄 Python-C++ Datenübertragung

### Datenstruktur (`Vehicle.h`)
```cpp
struct Point2D {
    float x, y;
};

struct VehicleDetectionData {
    Point2D front_position;
    Point2D rear_position;
    float angle;
    bool detected;
    std::string rear_color;
};
```

### pybind11 Bridge (`py_runner.cpp`)
```cpp
std::vector<VehicleDetectionData> get_all_vehicle_detections() {
    try {
        py::module_ camera_module = py::module_::import("MultiVehicleKamera");
        py::object result = camera_module.attr("detect_all_vehicles")();
        
        // Python-Liste zu C++ Vector konvertieren
        return result.cast<std::vector<VehicleDetectionData>>();
    } catch (const std::exception& e) {
        // Fallback: Leere Daten
        return std::vector<VehicleDetectionData>(4);
    }
}
```

## 🖥️ UI-Integration (Raylib)

### 2x2 Grid Layout
```cpp
void MultiCarDisplay::draw_vehicle_panel(int vehicle_index, 
                                        const VehicleDetectionData& data,
                                        int panel_x, int panel_y) {
    // Panel-Rahmen
    DrawRectangleLines(panel_x, panel_y, panel_width, panel_height, BLACK);
    
    // Fahrzeug-Status
    if (data.detected) {
        DrawText("ERKANNT", panel_x + 10, panel_y + 10, 20, GREEN);
        
        // Position anzeigen
        char pos_text[100];
        sprintf(pos_text, "Pos: (%.0f, %.0f)", data.front_position.x, data.front_position.y);
        DrawText(pos_text, panel_x + 10, panel_y + 40, 16, DARKGRAY);
        
        // Kompass zeichnen
        draw_compass(panel_x + panel_width - 80, panel_y + 20, data.angle);
    } else {
        DrawText("NICHT ERKANNT", panel_x + 10, panel_y + 10, 20, RED);
    }
}
```

### Kompass-Visualisierung
```cpp
void MultiCarDisplay::draw_compass(int center_x, int center_y, float angle) {
    const int radius = 30;
    
    // Kompass-Kreis
    DrawCircleLines(center_x, center_y, radius, BLACK);
    
    // Richtungspfeil
    float rad = (angle - 90) * PI / 180.0f;  // -90° für UI-Koordinaten
    int end_x = center_x + cos(rad) * (radius - 5);
    int end_y = center_y + sin(rad) * (radius - 5);
    
    DrawLine(center_x, center_y, end_x, end_y, RED);
    DrawCircle(end_x, end_y, 3, RED);
}
```

## 🔧 Konfiguration & Anpassung

### Kamera-Auswahl
```python
# Verschiedene Kamera-Indizes testen
cap = cv2.VideoCapture(0)  # Erste USB-Kamera
cap = cv2.VideoCapture(1)  # Zweite USB-Kamera
cap = cv2.VideoCapture(2)  # Dritte USB-Kamera
```

### HSV-Werte kalibrieren
Für verschiedene Lichtverhältnisse:
```python
# Hellere Umgebung - höhere V-Werte
lower_yellow = np.array([20, 100, 150])  # V: 150 statt 100

# Dunklere Umgebung - niedrigere V-Werte  
lower_yellow = np.array([20, 100, 80])   # V: 80 statt 100
```

### Erkennungsparameter
```python
MIN_CONTOUR_AREA = 100      # Mindestgröße für Farbpunkte
MAX_DISTANCE = 200          # Max. Abstand für Fahrzeug-Pairing
CAMERA_WIDTH = 640          # Kamera-Auflösung
CAMERA_HEIGHT = 480
```

## 🚀 Performance-Optimierung

### Frame-Rate-Kontrolle
```cpp
// In main.cpp - Raylib Game Loop
SetTargetFPS(30);  // 30 FPS für stabile Performance
```

### Memory Management
```python
# OpenCV Frame-Buffer Management
cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Reduziere Buffer-Lag
```

### Threading (Future Enhancement)
```cpp
// Potentielle Erweiterung: Async Vision Processing
std::async(std::launch::async, get_all_vehicle_detections);
```

## 🐛 Debugging & Troubleshooting

### Debug-Modus aktivieren
```python
DEBUG_MODE = True  # In MultiVehicleKamera.py

if DEBUG_MODE:
    cv2.imshow("Yellow Mask", mask_yellow)
    cv2.imshow("Red Mask", mask_red)
    # etc.
```

### Häufige Probleme

#### Keine Farberkennung
- HSV-Werte anpassen
- Beleuchtung verbessern
- Kamera-Fokus prüfen

#### Falsche Fahrzeug-Zuordnung  
- MAX_DISTANCE verringern
- Fahrzeuge weiter auseinander positionieren
- Doppelte Farbpunkte vermeiden

#### Performance-Probleme
- Kamera-Auflösung reduzieren
- FPS limitieren
- Unnötige Debug-Ausgaben entfernen

## 📊 Datenfluss-Diagramm

```
Kamera → OpenCV → HSV Filter → Contour Detection → Pairing Algorithm
   ↓                                                       ↓
USB Cap   ┌─ Gelb (Front)                            ┌─ Vehicle 1
   ↓      ├─ Rot (Heck)     →  Distance-Based  →     ├─ Vehicle 2  
Frame     ├─ Blau (Heck)        Assignment           ├─ Vehicle 3
   ↓      ├─ Grün (Heck)                             └─ Vehicle 4
Processing└─ Lila (Heck)                                   ↓
                                                     pybind11 Bridge
                                                           ↓
                                                    C++ Raylib Display
```

Die Kamera-Integration bildet das Herzstück des Multi-Vehicle Fleet Systems und ermöglicht robuste Echtzeit-Erkennung von bis zu 4 Fahrzeugen gleichzeitig.

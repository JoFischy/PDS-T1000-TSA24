# 📷 Kamera-Integration Dokumentation

## System-Übersicht

### Multi-Vehicle Detection System
**Aktueller Stand**: 4-Fahrzeug Echtzeit-Erkennung mit intelligenter Paar-Zuordnung und Beamer-Projektion

### Architektur-Prinzip
```
[Kamera] → [Python OpenCV] → [C++ Bridge] → [BeamerProjection 800x800]
    ↓              ↓               ↓                    ↓
HSV-Video   Farberkennung    Datenbridge        Boden-Projektion
```

## 🎨 Farb-Konfiguration

### Einheitliche Kopffarbe: ORANGE
```python
# Alle 4 Fahrzeuge haben orange Kopffarbe (UPDATE: von Rot zu Orange)
'front_hsv': ([5, 150, 150], [15, 255, 255])  # HSV-Bereich für Orange
```

### Identifikator-Farben (Heck)
```python
Auto-1: Orange-Kopf → Blau-Heck   ([100, 150, 50], [130, 255, 255])
Auto-2: Orange-Kopf → Grün-Heck   ([40, 100, 100], [80, 255, 255]) 
Auto-3: Orange-Kopf → Gelb-Heck   ([20, 100, 100], [30, 255, 255])
Auto-4: Orange-Kopf → Lila-Heck   ([130, 50, 50], [160, 255, 255])
```

## 🔍 Computer Vision Pipeline

### 1. Frame-Akquisition
```python
ret, frame = self.cap.read()  # 640x480 @ 30 FPS
hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
```

### 2. Farberkennung mit Mindestgröße-Filterung
```python
def find_all_color_centers(self, hsv_frame, lower_hsv, upper_hsv, is_front_color=False):
    # HSV-Maske erstellen
    mask = cv2.inRange(hsv_frame, np.array(lower_hsv), np.array(upper_hsv))
    
    # Morphologische Operationen (Rauschen entfernen)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
    
    # Mindestgröße-Filterung gegen Störungen
    min_area = MIN_FRONT_AREA if is_front_color else MIN_REAR_AREA
    
    # Konturen finden → Schwerpunkte berechnen (nur große Bereiche)
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
```

### 3. Intelligente Paar-Zuordnung
```python
def assign_closest_pairs(self, front_positions, rear_colors):
    # Für jede Identifikator-Farbe (UPDATE: front_positions statt red_positions)
    for vehicle in self.vehicles:
        # Finde nächsten orangen Punkt (noch nicht verwendet)
        min_distance = float('inf')
        for rear_pos in rear_colors[rear_color]:
            for front_pos in front_positions:
                if front_pos not in used_fronts:
                    distance = math.sqrt((front_pos[0] - rear_pos[0])**2 + 
                                       (front_pos[1] - rear_pos[1])**2)
```

### 4. Winkel-Berechnung
```python
def calculate_angle(self, front_pos, rear_pos):
    dx = front_pos[0] - rear_pos[0]
    dy = front_pos[1] - rear_pos[1]
    
    # atan2(dx, -dy) für 0° = nach oben
    angle_rad = math.atan2(dx, -dy)
    angle_deg = math.degrees(angle_rad)
    
    # Normalisiere zu 0-360°
    if angle_deg < 0:
        angle_deg += 360
```

## 🔗 Python-C++ Bridge

### Datenstruktur-Übertragung
```cpp
// C++ Seite (py_runner.cpp)
std::vector<VehicleDetectionData> get_all_vehicle_detections() {
    py::object detections = multi_vehicle_module.attr("get_multi_vehicle_detections")();
    
    for (auto item : detection_list) {
        py::dict detection = item.cast<py::dict>();
        
        VehicleDetectionData vehicle_data;
        // Position extrahieren (vereinfachte Struktur)
        py::dict position = detection["position"].cast<py::dict>();
        vehicle_data.position = Point2D(position["x"].cast<float>(), position["y"].cast<float>());
        
        vehicle_data.detected = detection["detected"].cast<bool>();
        vehicle_data.angle = detection["angle"].cast<float>();
        vehicle_data.distance = detection["distance"].cast<float>();
        vehicle_data.rear_color = detection["rear_color"].cast<std::string>();
    }
}
```

### Python Interface-Funktionen
```python
# Globale C++ Bridge Funktionen
def initialize_multi_vehicle_detection():    # Kamera-Init
def get_multi_vehicle_detections():          # Erkennungsdaten
def show_multi_vehicle_feed():               # Debug-Anzeige  
def cleanup_multi_vehicle_detection():       # Ressourcen-Cleanup
```

## 📊 Debug & Visualisierung

### Alle erkannten Punkte anzeigen
```python
def draw_all_detected_points(self, frame):
    # Orange Punkte (Kopffarbe) - UPDATE: von Rot zu Orange
    front_positions = self.find_all_color_centers(hsv, [5, 150, 150], [15, 255, 255], True)
    for pos in front_positions:
        cv2.circle(frame, pos, 6, (0, 165, 255), 2)  # Orange
        cv2.putText(frame, "O", (pos[0] + 8, pos[1] - 8), ...)
    
    # Identifikator-Farben  
    color_map = {
        'Blau': ([100, 150, 50], [130, 255, 255], (255, 0, 0)),
        'Grün': ([40, 100, 100], [80, 255, 255], (0, 255, 0)),
        'Gelb': ([20, 100, 100], [30, 255, 255], (0, 255, 255)),
        'Lila': ([130, 50, 50], [160, 255, 255], (255, 0, 255))
    }
```

### Kamera-Feed Informationen
- **Header**: "EINHEITLICHE VORDERE FARBE: ORANGE" (UPDATE)
- **Zähler**: "Erkannt: X/4 Autos"  
- **Debug**: "Alle Punkte: Y erkannt"
- **Status**: Fahrzeugname, Winkel, Identifikator-Farbe
- **Mindestgröße**: Front (100px), Heck (80px) für störungsfreie Erkennung

## ⚙️ Konfiguration & Anpassung

### Kamera-Einstellungen
```python
def initialize_camera(self):
    self.cap = cv2.VideoCapture(0)
    self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    self.cap.set(cv2.CAP_PROP_FPS, 30)
```

### HSV-Bereiche kalibrieren
Für optimale Erkennung bei verschiedenen Lichtverhältnissen:

1. **Orange (Kopf)**: H=5-15, S=150-255, V=150-255 (UPDATE: neue Hauptfarbe)
2. **Blau**: H=100-130, S=150-255, V=50-255  
3. **Grün**: H=40-80, S=100-255, V=100-255
4. **Gelb**: H=20-30, S=100-255, V=100-255
5. **Lila**: H=130-160, S=50-255, V=50-255

### Performance-Parameter
```python
# Mindestgröße für Farberkennung (konfigurierbar)
MIN_FRONT_AREA = 100  # Orange Front-Farbe (100 Pixel)
MIN_REAR_AREA = 80    # Heck-Identifikator-Farben (80 Pixel)

# Maximale Paar-Distanz  
if min_distance < 200:  # Pixel - größere Werte = toleranter
```

## 🔧 Erweiterte Features

### Morphologische Operationen
```python
# Rauschen entfernen
kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)   # Kleine Löcher schließen
mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)  # Kleine Objekte entfernen
```

### Intelligente Zuordnung
- **Used-Set Algorithmus**: Verhindert Doppel-Zuordnungen
- **Distanz-basiert**: Nächster verfügbarer Partner wird gewählt
- **Max-Distanz**: 200px Limit für realistische Paare
- **Robustheit**: Funktioniert auch bei teilweise verdeckten Objekten
- **Störungsfilterung**: Mindestflächen eliminieren Kamera-Artefakte

## 🎯 Beamer-Projektion Integration

### BeamerProjection System
```cpp
// 800x800 Quadratisches Fenster für Decken-Beamer
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

// Koordinaten-Transformation (Kamera → Projektion)
float proj_x = (pos.x / 640.0f) * WINDOW_WIDTH;
float proj_y = (pos.y / 480.0f) * WINDOW_HEIGHT;
```

### Visualisierung Features
- **Farbkodierte Rechtecke**: Jedes Fahrzeug in seiner Identifikator-Farbe
- **Richtungspfeile**: Zeigen Fahrtrichtung basierend auf Front-Heck-Ausrichtung  
- **Koordinaten-Legende**: Position und Koordinatensystem-Info
- **Weißer Hintergrund**: Optimiert für Boden-Projektion

## 🐛 Troubleshooting

### Häufige Probleme

1. **Keine Punkte erkannt**
   - HSV-Bereiche für Orange anpassen → [5,150,150] bis [15,255,255]
   - Mindestgröße zu hoch → MIN_FRONT_AREA/MIN_REAR_AREA reduzieren
   - Beleuchtung prüfen → Orange-Farbtöne bei aktueller Beleuchtung

2. **Kleine Störungen erkannt**  
   - Mindestgröße erhöhen → MIN_FRONT_AREA > 100px, MIN_REAR_AREA > 80px
   - Morphologische Operationen verstärken

3. **Falsche Zuordnungen**  
   - Distanz-Limit reduzieren → < 200px 
   - Eindeutige Farben sicherstellen

4. **Performance-Probleme**
   - Frame-Rate prüfen → 30 FPS bei 640x480
   - Beamer-Fenster Performance → 800x800 sollte flüssig laufen

### Debug-Kommandos
```python
# Python-Standalone Test
python src/MultiVehicleKamera.py

# VS Code Integration (empfohlen)
# F5 drücken für komplettes System mit Beamer-Projektion

# Konsole zeigt:
# "EINHEITLICHE VORDERE FARBE: ORANGE"
# "Alle Punkte: X erkannt" im Kamera-Feed
```

---
**Kamera-Integration für Multi-Vehicle Detection** | 📷 Computer Vision Pipeline

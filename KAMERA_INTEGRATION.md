# 📷 Kamera-Integration Dokumentation

## System-Übersicht

### Multi-Vehicle Detection System
**Aktueller Stand**: 4-Fahrzeug Echtzeit-Erkennung mit intelligenter Paar-Zuordnung

### Architektur-Prinzip
```
[Kamera] → [Python OpenCV] → [C++ Bridge] → [Raylib Display]
    ↓              ↓               ↓             ↓
HSV-Video   Farberkennung    Datenbridge   2x2 Grid UI
```

## 🎨 Farb-Konfiguration

### Einheitliche Kopffarbe: ROT
```python
# Alle 4 Fahrzeuge haben rote Kopffarbe
'front_hsv': ([0, 120, 70], [10, 255, 255])  # HSV-Bereich für Rot
```

### Identifikator-Farben (Heck)
```python
Auto-1: Rot-Kopf → Blau-Heck   ([100, 150, 50], [130, 255, 255])
Auto-2: Rot-Kopf → Grün-Heck   ([40, 100, 100], [80, 255, 255]) 
Auto-3: Rot-Kopf → Gelb-Heck   ([20, 100, 100], [30, 255, 255])
Auto-4: Rot-Kopf → Lila-Heck   ([130, 50, 50], [160, 255, 255])
```

## 🔍 Computer Vision Pipeline

### 1. Frame-Akquisition
```python
ret, frame = self.cap.read()  # 640x480 @ 30 FPS
hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
```

### 2. Farberkennung
```python
def find_all_color_centers(self, hsv_frame, lower_hsv, upper_hsv):
    # HSV-Maske erstellen
    mask = cv2.inRange(hsv_frame, np.array(lower_hsv), np.array(upper_hsv))
    
    # Morphologische Operationen (Rauschen entfernen)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
    
    # Konturen finden → Schwerpunkte berechnen
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
```

### 3. Intelligente Paar-Zuordnung
```python
def assign_closest_pairs(self, red_positions, rear_colors):
    # Für jede Identifikator-Farbe
    for vehicle in self.vehicles:
        # Finde nächsten roten Punkt (noch nicht verwendet)
        min_distance = float('inf')
        for rear_pos in rear_colors[rear_color]:
            for red_pos in red_positions:
                if red_pos not in used_reds:
                    distance = math.sqrt((red_pos[0] - rear_pos[0])**2 + 
                                       (red_pos[1] - rear_pos[1])**2)
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
    # Rote Punkte (Kopffarbe)
    red_positions = self.find_all_color_centers(hsv, [0, 120, 70], [10, 255, 255])
    for pos in red_positions:
        cv2.circle(frame, pos, 6, (0, 0, 255), 2)  # Rot
        cv2.putText(frame, "R", (pos[0] + 8, pos[1] - 8), ...)
    
    # Identifikator-Farben  
    color_map = {
        'Blau': ([100, 150, 50], [130, 255, 255], (255, 0, 0)),
        'Grün': ([40, 100, 100], [80, 255, 255], (0, 255, 0)),
        'Gelb': ([20, 100, 100], [30, 255, 255], (0, 255, 255)),
        'Lila': ([130, 50, 50], [160, 255, 255], (255, 0, 255))
    }
```

### Kamera-Feed Informationen
- **Header**: "EINHEITLICHE VORDERE FARBE: ROT"
- **Zähler**: "Erkannt: X/4 Autos"  
- **Debug**: "Alle Punkte: Y erkannt"
- **Status**: Fahrzeugname, Winkel, Identifikator-Farbe

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

1. **Rot (Kopf)**: H=0-10, S=120-255, V=70-255
2. **Blau**: H=100-130, S=150-255, V=50-255  
3. **Grün**: H=40-80, S=100-255, V=100-255
4. **Gelb**: H=20-30, S=100-255, V=100-255
5. **Lila**: H=130-160, S=50-255, V=50-255

### Performance-Parameter
```python
# Mindestgröße für Farberkennung
if area > 50:  # Pixel - kleinere Werte = empfindlicher

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

## 🐛 Troubleshooting

### Häufige Probleme

1. **Keine Punkte erkannt**
   - HSV-Bereiche zu eng → Werte anpassen
   - Zu dunkle Beleuchtung → area > 50 reduzieren
   - Kamera-Fokus → Autofokus prüfen

2. **Falsche Zuordnungen**  
   - Distanz-Limit zu hoch → < 200px reduzieren
   - Mehrere gleiche Farben → Eindeutige Farben wählen

3. **Performance-Probleme**
   - Frame-Rate reduzieren → FPS anpassen
   - Auflösung verringern → 320x240 testen

### Debug-Kommandos
```python
# Python-Standalone Test
python src/MultiVehicleKamera.py

# Alle erkannten Punkte werden im Feed angezeigt
# "Alle Punkte: X erkannt" zeigt Gesamtzahl
```

---
**Kamera-Integration für Multi-Vehicle Detection** | 📷 Computer Vision Pipeline

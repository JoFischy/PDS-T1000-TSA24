import cv2
import numpy as np
import math

class MultiVehicleDetector:
    def __init__(self):
        self.cap = None
        
        # Fahrzeug-Konfigurationen: Alle haben GELB vorne, nur hintere Farbe unterschiedlich
        self.vehicles = [
            {
                'name': 'Auto-1',
                'front_color': 'Gelb',
                'front_hsv': ([20, 100, 100], [30, 255, 255]),  # Gelb (einheitlich)
                'rear_color': 'Rot', 
                'rear_hsv': ([0, 120, 70], [10, 255, 255])       # Rot
            },
            {
                'name': 'Auto-2', 
                'front_color': 'Gelb',
                'front_hsv': ([20, 100, 100], [30, 255, 255]),  # Gelb (einheitlich)
                'rear_color': 'Blau',
                'rear_hsv': ([100, 150, 50], [130, 255, 255])    # Blau
            },
            {
                'name': 'Auto-3',
                'front_color': 'Gelb', 
                'front_hsv': ([20, 100, 100], [30, 255, 255]),  # Gelb (einheitlich)
                'rear_color': 'Grün',
                'rear_hsv': ([40, 100, 100], [80, 255, 255])     # Grün
            },
            {
                'name': 'Auto-4',
                'front_color': 'Gelb',
                'front_hsv': ([20, 100, 100], [30, 255, 255]),  # Gelb (einheitlich)
                'rear_color': 'Lila', 
                'rear_hsv': ([130, 50, 50], [160, 255, 255])     # Lila
            }
        ]
    
    def initialize_camera(self):
        """Initialisiert die Kamera"""
        try:
            self.cap = cv2.VideoCapture(0)
            if not self.cap.isOpened():
                print("Fehler: Kamera konnte nicht geöffnet werden!")
                return False
            
            # Kamera-Einstellungen
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            self.cap.set(cv2.CAP_PROP_FPS, 30)
            
            print("Multi-Vehicle Kamera initialisiert")
            return True
        except Exception as e:
            print(f"Kamera-Initialisierung fehlgeschlagen: {e}")
            return False
    
    def cleanup_camera(self):
        """Kamera-Ressourcen freigeben"""
        if self.cap is not None:
            self.cap.release()
            cv2.destroyAllWindows()
            print("Multi-Vehicle Kamera bereinigt")
    
    def find_all_colors(self, hsv_frame):
        """Findet alle gelben (vorne) und hinteren Farben im Bild"""
        try:
            # Alle gelben Punkte finden (vordere Farbe - einheitlich)
            yellow_positions = self.find_all_color_centers(hsv_frame, [20, 100, 100], [30, 255, 255])
            
            # Alle hinteren Farben finden
            rear_colors = {}
            for vehicle in self.vehicles:
                rear_name = vehicle['rear_color']
                rear_positions = self.find_all_color_centers(hsv_frame, vehicle['rear_hsv'][0], vehicle['rear_hsv'][1])
                if rear_positions:
                    rear_colors[rear_name] = rear_positions
            
            return yellow_positions, rear_colors
            
        except Exception as e:
            print(f"Fehler bei Farberkennung: {e}")
            return [], {}
    
    def find_all_color_centers(self, hsv_frame, lower_hsv, upper_hsv):
        """Findet alle Zentren einer bestimmten Farbe im HSV-Bild"""
        try:
            # HSV-Bereich anwenden
            mask = cv2.inRange(hsv_frame, np.array(lower_hsv), np.array(upper_hsv))
            
            # Rauschen entfernen
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
            mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
            mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
            
            # Konturen finden
            contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            positions = []
            for contour in contours:
                area = cv2.contourArea(contour)
                # Mindestgröße prüfen
                if area > 50:
                    # Schwerpunkt berechnen
                    M = cv2.moments(contour)
                    if M["m00"] != 0:
                        cx = int(M["m10"] / M["m00"])
                        cy = int(M["m01"] / M["m00"])
                        positions.append((cx, cy))
            
            return positions
        except Exception as e:
            print(f"Fehler bei Multi-Farberkennung: {e}")
            return []
    
    def assign_closest_pairs(self, yellow_positions, rear_colors):
        """Ordnet gelbe Punkte den nächstgelegenen hinteren Farben zu"""
        try:
            # Reset alle Fahrzeuge
            vehicle_assignments = {}
            for vehicle in self.vehicles:
                vehicle_assignments[vehicle['name']] = {
                    'front_pos': (-1, -1),
                    'rear_pos': (-1, -1),
                    'has_front': False,
                    'has_rear': False,
                    'distance': float('inf')
                }
            
            # Für jede hintere Farbe, finde das nächste gelbe
            used_yellows = set()
            
            for vehicle in self.vehicles:
                rear_color = vehicle['rear_color']
                vehicle_name = vehicle['name']
                
                if rear_color in rear_colors:
                    best_pair = None
                    min_distance = float('inf')
                    
                    # Für jeden hinteren Punkt dieser Farbe
                    for rear_pos in rear_colors[rear_color]:
                        # Finde das nächste gelbe, das noch nicht verwendet wurde
                        for yellow_pos in yellow_positions:
                            if yellow_pos not in used_yellows:
                                distance = math.sqrt((yellow_pos[0] - rear_pos[0])**2 + 
                                                   (yellow_pos[1] - rear_pos[1])**2)
                                if distance < min_distance:
                                    min_distance = distance
                                    best_pair = (yellow_pos, rear_pos)
                    
                    # Zuordnung speichern wenn gefunden
                    if best_pair and min_distance < 200:  # Max 200 Pixel Abstand
                        vehicle_assignments[vehicle_name] = {
                            'front_pos': best_pair[0],
                            'rear_pos': best_pair[1],
                            'has_front': True,
                            'has_rear': True,
                            'distance': min_distance
                        }
                        used_yellows.add(best_pair[0])
            
            return vehicle_assignments
            
        except Exception as e:
            print(f"Fehler bei Paar-Zuordnung: {e}")
            return {}
    
    def calculate_angle(self, front_pos, rear_pos):
        """Berechnet den Winkel zwischen vorderer und hinterer Position"""
        try:
            dx = front_pos[0] - rear_pos[0]
            dy = front_pos[1] - rear_pos[1]
            
            # atan2(dx, -dy) für 0° = nach oben
            angle_rad = math.atan2(dx, -dy)
            angle_deg = math.degrees(angle_rad)
            
            # Normalisiere zu 0-360°
            if angle_deg < 0:
                angle_deg += 360
                
            return angle_deg
        except Exception as e:
            print(f"Fehler bei Winkelberechnung: {e}")
            return 0.0
    
    def detect_all_vehicles(self):
        """Erkennt alle konfigurierten Fahrzeuge mit intelligenter Paar-Zuordnung"""
        if self.cap is None:
            print("Kamera nicht initialisiert!")
            return []
        
        try:
            ret, frame = self.cap.read()
            if not ret or frame is None:
                print("Kein Kamerabild empfangen!")
                return []
            
            # HSV-Konvertierung
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            
            # Alle Farben finden
            yellow_positions, rear_colors = self.find_all_colors(hsv)
            
            # Intelligente Zuordnung der nächsten Paare
            assignments = self.assign_closest_pairs(yellow_positions, rear_colors)
            
            # Ergebnisse im vereinfachten Format
            detections = []
            for vehicle in self.vehicles:
                vehicle_name = vehicle['name']
                assignment = assignments.get(vehicle_name, {
                    'front_pos': (-1, -1), 'rear_pos': (-1, -1),
                    'has_front': False, 'has_rear': False, 'distance': 0
                })
                
                # Vereinfachte Struktur: Nur Hauptposition + Distanz
                detection = {
                    'position': {'x': 0.0, 'y': 0.0},
                    'detected': False,
                    'angle': 0.0,
                    'distance': 0.0,
                    'rear_color': vehicle['rear_color']
                }
                
                # Berechne Hauptposition und Daten wenn beide Farben erkannt
                if assignment['has_front'] and assignment['has_rear']:
                    front_pos = assignment['front_pos']
                    rear_pos = assignment['rear_pos']
                    
                    # Hauptposition = Schwerpunkt zwischen Front und Heck
                    center_x = (front_pos[0] + rear_pos[0]) / 2.0
                    center_y = (front_pos[1] + rear_pos[1]) / 2.0
                    
                    detection['position'] = {'x': center_x, 'y': center_y}
                    detection['detected'] = True
                    detection['angle'] = self.calculate_angle(front_pos, rear_pos)
                    detection['distance'] = assignment['distance']
                
                detections.append(detection)
            
            return detections
            
        except Exception as e:
            print(f"Fehler bei intelligenter Fahrzeugerkennung: {e}")
            return []
    
    def show_camera_feed_with_detections(self):
        """Zeigt Kamera-Feed mit allen intelligenten Fahrzeugerkennungen"""
        if self.cap is None:
            print("Kamera nicht initialisiert!")
            return
        
        try:
            ret, frame = self.cap.read()
            if not ret or frame is None:
                return
            
            # Erkenne alle Fahrzeuge mit intelligenter Zuordnung
            detections = self.detect_all_vehicles()
            
            # Zähle erkannte Fahrzeuge
            detected_count = sum(1 for d in detections if d['detected'])
            
            # Header-Info
            cv2.putText(frame, f"EINHEITLICHE VORDERE FARBE: GELB", (10, 25), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
            cv2.putText(frame, f"Erkannt: {detected_count}/4 Autos", (10, 50), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
            
            # Zeichne Erkennungen
            for i, detection in enumerate(detections):
                vehicle_name = f"Auto-{i+1}"
                # Fahrzeugname und Status (rechts)
                status_text = f"{vehicle_name}: "
                if detection['detected']:
                    status_text += f"{int(detection['angle'])}° - {detection['rear_color']}"
                else:
                    status_text += f"Nicht erkannt - {detection['rear_color']}"
                
                cv2.putText(frame, status_text, (frame.shape[1] - 300, 30 + i * 25), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
                
                # Zeichne erkanntes Fahrzeug (vereinfachte Darstellung)
                if detection['detected']:
                    # Hauptposition als großer Kreis
                    center_x = int(detection['position']['x'])
                    center_y = int(detection['position']['y'])
                    
                    # Fahrzeug-spezifische Farbe
                    if i == 0: vehicle_color = (0, 0, 255)      # Rot für Auto-1
                    elif i == 1: vehicle_color = (255, 0, 0)    # Blau für Auto-2
                    elif i == 2: vehicle_color = (0, 255, 0)    # Grün für Auto-3
                    else: vehicle_color = (255, 0, 255)         # Lila für Auto-4
                    
                    # Hauptkreis
                    cv2.circle(frame, (center_x, center_y), 12, vehicle_color, 3)
                    cv2.putText(frame, vehicle_name, (center_x + 15, center_y - 5), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.5, vehicle_color, 2)
                    
                    # Richtungspfeil
                    import math
                    angle_rad = math.radians(detection['angle'])
                    arrow_length = 20
                    arrow_end = (
                        int(center_x + arrow_length * math.cos(angle_rad)),
                        int(center_y + arrow_length * math.sin(angle_rad))
                    )
                    cv2.arrowedLine(frame, (center_x, center_y), arrow_end, vehicle_color, 3)
                    
                    # Distanz anzeigen
                    dist_text = f"{detection['distance']:.0f}px"
                    cv2.putText(frame, dist_text, (center_x - 10, center_y + 25), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.4, vehicle_color, 1)
            
            # Anweisungen
            cv2.putText(frame, "ESC = Beenden | Vereinfachte Hauptposition + Distanz", 
                       (10, frame.shape[0] - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
            
            cv2.imshow("Multi-Vehicle Detection - Vereinfachte Darstellung", frame)
            
        except Exception as e:
            print(f"Fehler bei intelligentem Kamera-Feed: {e}")

# Globale Instanz für C++ Interface
global_detector = None

def initialize_multi_vehicle_detection():
    """Initialisiert Multi-Vehicle Detection System"""
    global global_detector
    try:
        global_detector = MultiVehicleDetector()
        return global_detector.initialize_camera()
    except Exception as e:
        print(f"Multi-Vehicle Initialisierung fehlgeschlagen: {e}")
        return False

def get_multi_vehicle_detections():
    """Gibt alle Fahrzeugerrkennungen zurück"""
    global global_detector
    if global_detector is None:
        print("Multi-Vehicle Detector nicht initialisiert!")
        return []
    
    return global_detector.detect_all_vehicles()

def show_multi_vehicle_feed():
    """Zeigt Multi-Vehicle Kamera-Feed"""
    global global_detector
    if global_detector is None:
        print("Multi-Vehicle Detector nicht initialisiert!")
        return
    
    global_detector.show_camera_feed_with_detections()

def cleanup_multi_vehicle_detection():
    """Bereinigt Multi-Vehicle Detection System"""
    global global_detector
    if global_detector is not None:
        global_detector.cleanup_camera()
        global_detector = None

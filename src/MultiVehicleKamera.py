import cv2
import numpy as np
import math

class MultiVehicleDetector:
    """
    Multi-Vehicle Detection System mit intelligenter Paar-Zuordnung
    
    Erkennt 4 Fahrzeuge basierend auf:
    - Einheitliche ORANGE-Kopffarbe (alle Fahrzeuge)
    - Individuelle Identifikator-Farben am Heck (Blau, Grün, Gelb, Lila)
    """
    def __init__(self):
        self.cap = None
        
        # Mindestgröße für Farberkennung (in Pixeln)
        self.MIN_FRONT_AREA = 100   # Orange-Bereiche müssen mindestens 100 Pixel groß sein
        self.MIN_REAR_AREA = 80     # Heck-Farben müssen mindestens 80 Pixel groß sein
        
        # Fahrzeug-Konfigurationen: ORANGE-Kopf + individuelle Heck-Identifikatoren
        self.vehicles = [
            {
                'name': 'Auto-1',
                'front_color': 'Orange',
                'front_hsv': ([5, 150, 150], [15, 255, 255]),     # Orange (einheitlich)
                'rear_color': 'Blau', 
                'rear_hsv': ([100, 150, 50], [130, 255, 255])    # Blau-Identifikator
            },
            {
                'name': 'Auto-2', 
                'front_color': 'Orange',
                'front_hsv': ([5, 150, 150], [15, 255, 255]),     # Orange (einheitlich)
                'rear_color': 'Grün',
                'rear_hsv': ([40, 100, 100], [80, 255, 255])     # Grün-Identifikator
            },
            {
                'name': 'Auto-3',
                'front_color': 'Orange', 
                'front_hsv': ([5, 150, 150], [15, 255, 255]),     # Orange (einheitlich)
                'rear_color': 'Gelb',
                'rear_hsv': ([20, 100, 100], [30, 255, 255])     # Gelb-Identifikator
            },
            {
                'name': 'Auto-4',
                'front_color': 'Orange',
                'front_hsv': ([5, 150, 150], [15, 255, 255]),     # Orange (einheitlich)
                'rear_color': 'Lila', 
                'rear_hsv': ([130, 50, 50], [160, 255, 255])     # Lila-Identifikator
            }
        ]
    
    def initialize_camera(self):
        """Initialisiert die Kamera"""
        try:
            # Versuche verschiedene Kamera-Indizes
            for camera_index in [0, 1, 2]:
                print(f"Versuche Kamera-Index {camera_index}...")
                self.cap = cv2.VideoCapture(camera_index)
                
                if self.cap.isOpened():
                    # Teste ob ein Frame gelesen werden kann
                    ret, frame = self.cap.read()
                    if ret and frame is not None:
                        print(f"Kamera erfolgreich initialisiert (Index: {camera_index})")
                        # Setze Kamera-Eigenschaften für bessere Performance
                        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
                        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
                        self.cap.set(cv2.CAP_PROP_FPS, 30)
                        return True
                    else:
                        self.cap.release()
                        self.cap = None
                else:
                    self.cap = None
            
            print("Keine funktionierende Kamera gefunden!")
            return False
            
        except Exception as e:
            print(f"Kamera-Initialisierung fehlgeschlagen: {e}")
            if self.cap is not None:
                self.cap.release()
                self.cap = None
            return False
    
    def cleanup_camera(self):
        """Bereinigt Ressourcen"""
        if self.cap is not None:
            self.cap.release()
            cv2.destroyAllWindows()
        print("Multi-Vehicle Detection System bereinigt")
    
    def find_all_colors(self, hsv_frame):
        """
        Findet alle orange Kopf-Punkte und Identifikator-Farben im Bild
        
        Returns:
            tuple: (front_positions, rear_colors_dict)
        """
        try:
            # Alle orange Punkte finden (vordere Farbe - einheitlich)
            front_positions = self.find_all_color_centers(hsv_frame, [5, 150, 150], [15, 255, 255], is_front_color=True)
            
            # Alle hinteren Farben finden
            rear_colors = {}
            for vehicle in self.vehicles:
                rear_name = vehicle['rear_color']
                rear_positions = self.find_all_color_centers(hsv_frame, vehicle['rear_hsv'][0], vehicle['rear_hsv'][1], is_front_color=False)
                if rear_positions:
                    rear_colors[rear_name] = rear_positions
            
            return front_positions, rear_colors
            
        except Exception as e:
            print(f"Fehler bei Farberkennung: {e}")
            return [], {}
    
    def find_all_color_centers(self, hsv_frame, lower_hsv, upper_hsv, is_front_color=False):
        """Findet alle Zentren einer bestimmten Farbe im HSV-Bild mit konfigurierbarer Mindestgröße"""
        try:
            # HSV-Bereich anwenden
            mask = cv2.inRange(hsv_frame, np.array(lower_hsv), np.array(upper_hsv))
            
            # Rauschen entfernen
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
            mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
            mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
            
            # Konturen finden
            contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            # Wähle Mindestgröße basierend auf Farb-Typ
            min_area = self.MIN_FRONT_AREA if is_front_color else self.MIN_REAR_AREA
            
            positions = []
            for contour in contours:
                area = cv2.contourArea(contour)
                # Mindestgröße prüfen (konfigurierbar)
                if area > min_area:
                    # Schwerpunkt berechnen
                    M = cv2.moments(contour)
                    if M["m00"] != 0:
                        cx = int(M["m10"] / M["m00"])
                        cy = int(M["m01"] / M["m00"])
                        positions.append((cx, cy))
                        
                        # Debug-Info für große Bereiche
                        if area > min_area * 2:
                            color_type = "FRONT" if is_front_color else "REAR"
                            print(f"Große {color_type}-Farbe erkannt: {area:.0f} Pixel bei ({cx}, {cy})")
            
            return positions
        except Exception as e:
            print(f"Fehler bei Multi-Farberkennung: {e}")
            return []
    
    def assign_closest_pairs(self, front_positions, rear_colors):
        """
        Intelligente Paar-Zuordnung basierend auf minimaler Distanz
        
        Ordnet jeden orangen Punkt (Kopf) dem nächstgelegenen Identifikator zu.
        Verhindert Doppel-Zuordnungen durch Used-Set Algorithmus.
        
        Args:
            front_positions: Liste der erkannten orangen Punkte
            rear_colors: Dict mit Identifikator-Farben und deren Positionen
            
        Returns:
            dict: Zuordnungen pro Fahrzeug mit Positionen und Distanzen
        """
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
            
            # Für jede hintere Farbe, finde den nächsten orangen Punkt
            used_fronts = set()
            
            for vehicle in self.vehicles:
                rear_color = vehicle['rear_color']
                vehicle_name = vehicle['name']
                
                if rear_color in rear_colors:
                    best_pair = None
                    min_distance = float('inf')
                    
                    # Für jeden hinteren Punkt dieser Farbe
                    for rear_pos in rear_colors[rear_color]:
                        # Finde das nächste orange, das noch nicht verwendet wurde
                        for front_pos in front_positions:
                            if front_pos not in used_fronts:
                                distance = math.sqrt((front_pos[0] - rear_pos[0])**2 + 
                                                   (front_pos[1] - rear_pos[1])**2)
                                if distance < min_distance:
                                    min_distance = distance
                                    best_pair = (front_pos, rear_pos)
                    
                    # Zuordnung speichern wenn gefunden
                    if best_pair and min_distance < 200:  # Max 200 Pixel Abstand
                        vehicle_assignments[vehicle_name] = {
                            'front_pos': best_pair[0],
                            'rear_pos': best_pair[1],
                            'has_front': True,
                            'has_rear': True,
                            'distance': min_distance
                        }
                        used_fronts.add(best_pair[0])
            
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
    
    def detect_all_vehicles(self, input_frame=None):
        """Erkennt alle konfigurierten Fahrzeuge mit intelligenter Paar-Zuordnung"""
        if input_frame is None and self.cap is None:
            print("Kein Eingabebild und keine Kamera verfügbar!")
            return []
        
        try:
            # Verwende Eingabebild oder Kamera
            if input_frame is not None:
                frame = input_frame
            else:
                ret, frame = self.cap.read()
                if not ret or frame is None:
                    print("Kein Kamerabild empfangen!")
                    return []
            
            # HSV-Konvertierung
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            
            # Alle Farben finden
            front_positions, rear_colors = self.find_all_colors(hsv)
            
            # Intelligente Zuordnung der nächsten Paare
            assignments = self.assign_closest_pairs(front_positions, rear_colors)
            
            # Ergebnisse formatieren
            detections = []
            for vehicle in self.vehicles:
                vehicle_name = vehicle['name']
                assignment = assignments.get(vehicle_name, {
                    'front_pos': (-1, -1), 'rear_pos': (-1, -1),
                    'has_front': False, 'has_rear': False, 'distance': 0
                })
                
                detection = {
                    'vehicle_name': vehicle_name,
                    'front_color': vehicle['front_color'],
                    'rear_color': vehicle['rear_color'],
                    'front_pos': (float(assignment['front_pos'][0]), float(assignment['front_pos'][1])),
                    'rear_pos': (float(assignment['rear_pos'][0]), float(assignment['rear_pos'][1])),
                    'has_front': assignment['has_front'],
                    'has_rear': assignment['has_rear'],
                    'has_angle': False,
                    'angle_degrees': 0.0,
                    'distance_pixels': assignment.get('distance', 0.0)
                }
                
                # Berechne Richtung wenn beide Farben erkannt
                if assignment['has_front'] and assignment['has_rear']:
                    detection['angle_degrees'] = self.calculate_angle(assignment['front_pos'], assignment['rear_pos'])
                    detection['distance_pixels'] = assignment['distance']
                    detection['has_angle'] = True
                
                detections.append(detection)
            
            return detections
            
        except Exception as e:
            print(f"Fehler bei intelligenter Fahrzeugerkennung: {e}")
            return []
    
    def show_camera_feed_with_detections(self, input_frame=None):
        """Zeigt Kamera-Feed mit allen intelligenten Fahrzeugerkennungen (optional mit Eingabebild)"""
        if input_frame is None and self.cap is None:
            print("Kein Eingabebild und keine Kamera verfügbar!")
            return
        
        try:
            # Verwende Eingabebild oder Kamera
            if input_frame is not None:
                frame = input_frame
            else:
                ret, frame = self.cap.read()
                if not ret or frame is None:
                    return
            
            # Erkenne alle Fahrzeuge mit intelligenter Zuordnung
            detections = self.detect_all_vehicles(frame)
            
            # MARKIERE ALLE ERKANNTEN FARBPUNKTE (für Debugging)
            self.draw_all_detected_points(frame)
            
            # Zähle erkannte Fahrzeuge
            detected_count = sum(1 for d in detections if d['has_angle'])
            
            # Header-Info
            cv2.putText(frame, f"EINHEITLICHE VORDERE FARBE: ORANGE", (10, 25), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 165, 255), 2)
            cv2.putText(frame, f"Erkannt: {detected_count}/4 Autos", (10, 50), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
            
            # Zeichne Erkennungen
            for i, detection in enumerate(detections):
                # Fahrzeugname und Status (rechts)
                status_text = f"{detection['vehicle_name']}: "
                if detection['has_angle']:
                    status_text += f"{int(detection['angle_degrees'])}° - {detection['rear_color']}"
                else:
                    status_text += f"Nicht erkannt - {detection['rear_color']}"
                
                cv2.putText(frame, status_text, (frame.shape[1] - 300, 30 + i * 25), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
                
                # Zeichne erkannte Punkte
                if detection['has_front']:
                    cv2.circle(frame, (int(detection['front_pos'][0]), int(detection['front_pos'][1])), 
                              8, (0, 165, 255), 3)  # Orange für vorne (einheitlich)
                    cv2.putText(frame, "ORANGE", (int(detection['front_pos'][0]) + 12, int(detection['front_pos'][1]) - 5), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 165, 255), 1)
                
                if detection['has_rear']:
                    # Farbe je nach Fahrzeug
                    if detection['rear_color'] == 'Rot':
                        color = (0, 0, 255)
                    elif detection['rear_color'] == 'Blau':
                        color = (255, 0, 0)
                    elif detection['rear_color'] == 'Grün':
                        color = (0, 255, 0)
                    elif detection['rear_color'] == 'Lila':
                        color = (255, 0, 255)
                    else:
                        color = (128, 128, 128)
                    
                    cv2.circle(frame, (int(detection['rear_pos'][0]), int(detection['rear_pos'][1])), 
                              8, color, 3)
                    cv2.putText(frame, detection['rear_color'], 
                               (int(detection['rear_pos'][0]) + 12, int(detection['rear_pos'][1]) - 5), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.4, color, 1)
                
                # Zeichne Verbindungslinie und Richtungspfeil
                if detection['has_angle']:
                    front_pt = (int(detection['front_pos'][0]), int(detection['front_pos'][1]))
                    rear_pt = (int(detection['rear_pos'][0]), int(detection['rear_pos'][1]))
                    
                    # Fahrzeug-spezifische Farbe für Linie
                    if i == 0: line_color = (255, 255, 0)      # Cyan für Auto-1
                    elif i == 1: line_color = (255, 128, 0)    # Orange für Auto-2
                    elif i == 2: line_color = (128, 255, 0)    # Hellgrün für Auto-3
                    else: line_color = (255, 0, 128)           # Pink für Auto-4
                    
                    cv2.line(frame, rear_pt, front_pt, line_color, 2)
                    
                    # Richtungspfeil
                    angle_rad = math.radians(detection['angle_degrees'])
                    arrow_end = (
                        int(front_pt[0] + 15 * math.sin(angle_rad)),
                        int(front_pt[1] - 15 * math.cos(angle_rad))
                    )
                    cv2.arrowedLine(frame, front_pt, arrow_end, line_color, 2)
                    
                    # Abstand anzeigen
                    mid_x = (front_pt[0] + rear_pt[0]) // 2
                    mid_y = (front_pt[1] + rear_pt[1]) // 2
                    cv2.putText(frame, f"{detection['distance_pixels']:.0f}px", 
                               (mid_x + 5, mid_y), cv2.FONT_HERSHEY_SIMPLEX, 0.3, line_color, 1)
            
            # Anweisungen
            cv2.putText(frame, "ESC = Beenden | Intelligente Paar-Zuordnung aktiv", 
                       (10, frame.shape[0] - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
            
            cv2.imshow("Multi-Vehicle Detection - Einheitlich Rot Vorne", frame)
            
        except Exception as e:
            print(f"Fehler bei intelligentem Kamera-Feed: {e}")
    
    def draw_all_detected_points(self, frame):
        """Markiert ALLE erkannten Farbpunkte im Bild zur Debugging-Zwecken"""
        try:
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            
            # Erkenne alle orangen Punkte (Kopffarbe)
            front_positions = self.find_all_color_centers(hsv, [5, 150, 150], [15, 255, 255], is_front_color=True)
            for pos in front_positions:
                cv2.circle(frame, pos, 6, (0, 165, 255), 2)  # Orange für Kopf
                cv2.putText(frame, "O", (pos[0] + 8, pos[1] - 8), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.3, (0, 165, 255), 1)
            
            # Erkenne alle Identifikator-Farben
            color_map = {
                'Blau': ([100, 150, 50], [130, 255, 255], (255, 0, 0)),
                'Grün': ([40, 100, 100], [80, 255, 255], (0, 255, 0)),
                'Gelb': ([20, 100, 100], [30, 255, 255], (0, 255, 255)),
                'Lila': ([130, 50, 50], [160, 255, 255], (255, 0, 255))
            }
            
            for color_name, (lower, upper, draw_color) in color_map.items():
                positions = self.find_all_color_centers(hsv, lower, upper)
                for pos in positions:
                    cv2.circle(frame, pos, 6, draw_color, 2)
                    cv2.putText(frame, color_name[0], (pos[0] + 8, pos[1] - 8), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.3, draw_color, 1)
            
            # Info-Text
            total_points = len(front_positions) + sum(len(self.find_all_color_centers(hsv, lower, upper, is_front_color=False)) 
                                                   for _, (lower, upper, _) in color_map.items())
            cv2.putText(frame, f"Alle Punkte: {total_points} erkannt", (10, 75), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
            
        except Exception as e:
            print(f"Fehler beim Markieren aller Punkte: {e}")

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

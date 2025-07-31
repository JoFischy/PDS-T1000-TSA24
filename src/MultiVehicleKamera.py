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
        
        # STABILES ERKENNUNGSRECHTECK - fest definiert nach erster Erkennung
        self.stable_detection_area = None  # Wird beim ersten Mal gesetzt und bleibt fix
        
        # Fahrzeug-Konfigurationen: ORANGE-Kopf + individuelle Heck-Identifikatoren
        # ANGEPASSTE HSV-Bereiche basierend auf echten Farbwerten
        self.vehicles = [
            {
                'name': 'Auto-1',
                'front_color': 'Orange',
                'front_hsv': ([8, 180, 220], [18, 255, 255]),      # Orange: H:13±5, S:205±25, V:250+
                'rear_color': 'Blau', 
                'rear_hsv': ([100, 80, 30], [130, 255, 255])     # Blau erweitert (heller)
            },
            {
                'name': 'Auto-2', 
                'front_color': 'Orange',
                'front_hsv': ([8, 180, 220], [18, 255, 255]),      # Orange: H:13±5, S:205±25, V:250+
                'rear_color': 'Grün',
                'rear_hsv': ([40, 60, 50], [80, 255, 255])       # Grün erweitert (heller)
            },
            {
                'name': 'Auto-3',
                'front_color': 'Orange', 
                'front_hsv': ([8, 180, 220], [18, 255, 255]),      # Orange: H:13±5, S:205±25, V:250+
                'rear_color': 'Gelb',
                'rear_hsv': ([25, 80, 180], [35, 255, 255])       # Gelb: NICHT H:95 (falsches Gelb ausschließen)
            },
            {
                'name': 'Auto-4',
                'front_color': 'Orange',
                'front_hsv': ([8, 180, 220], [18, 255, 255]),      # Orange: H:13±5, S:205±25, V:250+
                'rear_color': 'Lila', 
                'rear_hsv': ([130, 40, 40], [165, 255, 255])     # Lila erweitert (heller)
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
                        
                        # HANDY-KAMERA: Vollständige manuelle Kontrolle für stabile Farberkennung
                        print("Aktiviere einmaligen Autofokus für Handy-Kamera...")
                        self.cap.set(cv2.CAP_PROP_AUTOFOCUS, 1)  # Einmal fokussieren
                        
                        # Kurz warten für Fokussierung (3 Sekunden)
                        import time
                        time.sleep(3)
                        
                        # ALLE automatischen Anpassungen deaktivieren
                        self.cap.set(cv2.CAP_PROP_AUTOFOCUS, 0)          # Fester Fokus
                        self.cap.set(cv2.CAP_PROP_AUTO_EXPOSURE, 0.25)   # Manuelle Belichtung (25% = manuell)
                        self.cap.set(cv2.CAP_PROP_EXPOSURE, -6)          # Feste Belichtung (-6 = mittlere Helligkeit)
                        self.cap.set(cv2.CAP_PROP_AUTO_WB, 0)            # Weißabgleich aus
                        self.cap.set(cv2.CAP_PROP_WB_TEMPERATURE, 5000)  # Feste Farbtemperatur (Tageslicht)
                        self.cap.set(cv2.CAP_PROP_GAIN, 0)               # Kein automatischer Gain
                        self.cap.set(cv2.CAP_PROP_BRIGHTNESS, 128)       # Mittlere Helligkeit
                        self.cap.set(cv2.CAP_PROP_CONTRAST, 128)         # Mittlerer Kontrast
                        self.cap.set(cv2.CAP_PROP_SATURATION, 150)       # Leicht erhöhte Sättigung für Farberkennung
                        print("Alle Kamera-Parameter manuell fixiert - konstante Farberkennung aktiviert")
                        
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
            # Alle orange Punkte finden (vordere Farbe - einheitlich) - ANGEPASST auf echte Orange-Werte
            front_positions = self.find_all_color_centers(hsv_frame, [8, 180, 220], [18, 255, 255], is_front_color=True)
            
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
            
            # WICHTIG: Entferne ROTE Bereiche aus der Fahrzeug-Farberkennung (ERWEITERT für helle Beleuchtung)
            # um Konflikte mit den Beamer-Eckpunkten zu vermeiden
            red_lower1 = np.array([0, 80, 80])      # Rot-Bereich 1 (heller)
            red_upper1 = np.array([10, 255, 255])
            red_lower2 = np.array([170, 80, 80])    # Rot-Bereich 2 (heller)
            red_upper2 = np.array([180, 255, 255])
            red_mask1 = cv2.inRange(hsv_frame, red_lower1, red_upper1)
            red_mask2 = cv2.inRange(hsv_frame, red_lower2, red_upper2)
            red_combined = cv2.add(red_mask1, red_mask2)
            
            # Subtrahiere ROTE Bereiche von der Fahrzeugfarben-Maske
            mask = cv2.bitwise_and(mask, cv2.bitwise_not(red_combined))
            
            # ZUSÄTZLICH: Entferne sehr helle/weiße Bereiche (Mittelpunkt-Problem)
            # Prüfe auf zu niedrige Sättigung (weiße/graue Bereiche)
            _, saturation, value = cv2.split(hsv_frame)
            low_saturation_mask = saturation < 30  # Niedrige Sättigung = Weiß/Grau
            high_value_mask = value > 200         # Hohe Helligkeit = sehr hell
            
            # Entferne Bereiche mit niedriger Sättigung ODER sehr hoher Helligkeit
            white_gray_mask = np.logical_or(low_saturation_mask, high_value_mask)
            mask = cv2.bitwise_and(mask, (~white_gray_mask).astype(np.uint8) * 255)
            
            # Strengere Rauschen-Entfernung
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (7, 7))
            mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel, iterations=2)
            mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel, iterations=1)
            
            # Konturen finden
            contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            # Höhere Mindestgröße für bessere Filterung
            min_area = self.MIN_FRONT_AREA * 1.5 if is_front_color else self.MIN_REAR_AREA * 1.5
            
            positions = []
            for contour in contours:
                area = cv2.contourArea(contour)
                # Mindestgröße prüfen (strenger)
                if area > min_area:
                    # Schwerpunkt berechnen
                    M = cv2.moments(contour)
                    if M["m00"] != 0:
                        cx = int(M["m10"] / M["m00"])
                        cy = int(M["m01"] / M["m00"])
                        
                        # NEUE PRÜFUNG: Nur Punkte innerhalb des Beamer-Erkennungsbereichs
                        if self.is_point_in_detection_area(cx, cy):
                            # ZUSÄTZLICHE PRÜFUNG: Mindest-Abstand zu Rahmen-Mittelpunkt
                            if hasattr(self, 'detection_area') and self.detection_area.get('use_mask', False):
                                # Bei maskenbasierter Erkennung: Prüfe Abstand zu Zentrum
                                center_x = (self.detection_area['x_min'] + self.detection_area['x_max']) // 2
                                center_y = (self.detection_area['y_min'] + self.detection_area['y_max']) // 2
                                distance_to_center = ((cx - center_x)**2 + (cy - center_y)**2)**0.5
                                
                                # Mindestabstand zum Zentrum (verhindert Mittelpunkt-Erkennungen)
                                min_distance_from_center = 30
                                if distance_to_center < min_distance_from_center:
                                    print(f"Punkt bei ({cx},{cy}) zu nah am Zentrum ({center_x},{center_y}) - ignoriert")
                                    continue
                            
                            positions.append((cx, cy))
                            
                            # Debug-Info für große Bereiche
                            if area > min_area * 1.5:
                                color_type = "FRONT" if is_front_color else "REAR"
                                print(f"Große {color_type}-Farbe erkannt: {area:.0f} Pixel bei ({cx}, {cy}) - INNERHALB Bereich")
                        else:
                            # Debug-Info für ignorierte Punkte
                            if area > min_area * 1.5:
                                color_type = "FRONT" if is_front_color else "REAR"
                                print(f"Große {color_type}-Farbe IGNORIERT: {area:.0f} Pixel bei ({cx}, {cy}) - AUßERHALB Bereich")
            
            return positions
        except Exception as e:
            print(f"Fehler bei Multi-Farberkennung: {e}")
            return []
    
    def detect_beamer_frame_in_image(self, frame):
        """Erkennt 4 ROTE Eckpunkte im Kamerabild und spannt zwischen ihnen den Erkennungsbereich auf"""
        try:
            height, width = frame.shape[:2]
            
            # WENN bereits ein stabiles Erkennungsgebiet existiert, verwende es
            if self.stable_detection_area is not None:
                self.detection_area = self.stable_detection_area.copy()
                
                # Zeichne das stabile Rechteck zur Visualisierung
                if 'corners' in self.detection_area:
                    x_min, y_min = self.detection_area['x_min'], self.detection_area['y_min']
                    x_max, y_max = self.detection_area['x_max'], self.detection_area['y_max']
                    cv2.rectangle(frame, (x_min, y_min), (x_max, y_max), (0, 255, 0), 3)
                    cv2.putText(frame, "STABILER ERKENNUNGSBEREICH", (x_min + 5, y_min - 10), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
                    
                    # Zeichne auch die 4 Eckpunkte
                    corners = self.detection_area['corners']
                    for i, corner in enumerate(corners):
                        cv2.circle(frame, corner, 8, (0, 255, 0), -1)
                
                cv2.putText(frame, f"STABILER BEREICH", (10, 30), 
                           cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 3)
                
                return True
            
            # HSV-Konvertierung für Farberkennung
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            
            # ROT Farberkennung für Eckpunkte (ERWEITERT für helle Beleuchtung)
            # ROT in HSV: Hue ~0° und ~180°, REDUZIERTE Sättigung für hellere Varianten
            lower_red1 = np.array([0, 80, 80])       # Rot-Bereich 1: 0-10° (heller)
            upper_red1 = np.array([10, 255, 255])
            lower_red2 = np.array([170, 80, 80])     # Rot-Bereich 2: 170-180° (heller)
            upper_red2 = np.array([180, 255, 255])
            
            # ROT Maske erstellen (beide Rot-Bereiche kombinieren)
            red_mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
            red_mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
            red_mask = cv2.add(red_mask1, red_mask2)
            
            # Morphologische Operationen für bessere Punkt-Erkennung
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
            red_mask = cv2.morphologyEx(red_mask, cv2.MORPH_OPEN, kernel)
            red_mask = cv2.morphologyEx(red_mask, cv2.MORPH_CLOSE, kernel)
            
            # DEBUG: Zeige ROT Maske zur Visualisierung
            cv2.imshow("ROTE Eckpunkt-Maske", red_mask)
            
            # Finde Konturen (sollten die 4 Eckpunkte sein)
            contours, _ = cv2.findContours(red_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            if len(contours) < 4:
                print(f"WARNUNG: Nur {len(contours)} ROTE Eckpunkte gefunden! Benötige 4. Verwende Vollbild.")
                self.detection_area = {
                    'x_min': 0, 'y_min': 0,
                    'x_max': width, 'y_max': height,
                    'use_mask': False
                }
                self.detection_mask = None
                return False
            
            # Finde die 4 größten Konturen (Eckpunkte)
            corner_centers = []
            for contour in contours:
                area = cv2.contourArea(contour)
                if area > 50:  # Mindestgröße für Eckpunkte
                    # Berechne Zentrum des Eckpunkts
                    M = cv2.moments(contour)
                    if M["m00"] != 0:
                        cx = int(M["m10"] / M["m00"])
                        cy = int(M["m01"] / M["m00"])
                        corner_centers.append((cx, cy, area))
            
            # Sortiere nach Größe und nimm die 4 größten
            corner_centers.sort(key=lambda x: x[2], reverse=True)
            corner_centers = corner_centers[:4]
            
            if len(corner_centers) < 4:
                print(f"WARNUNG: Nur {len(corner_centers)} gültige ROTE Eckpunkte! Verwende Vollbild.")
                self.detection_area = {
                    'x_min': 0, 'y_min': 0,
                    'x_max': width, 'y_max': height,
                    'use_mask': False
                }
                self.detection_mask = None
                return False
            
            # Extrahiere nur die Koordinaten (ohne Fläche)
            corners = [(x, y) for x, y, area in corner_centers]
            
            # Sortiere Eckpunkte: Oben-Links, Oben-Rechts, Unten-Links, Unten-Rechts
            corners.sort(key=lambda p: p[1])  # Sortiere nach Y-Koordinate
            top_corners = corners[:2]
            bottom_corners = corners[2:4]
            
            # Sortiere obere und untere Eckpunkte nach X-Koordinate
            top_corners.sort(key=lambda p: p[0])
            bottom_corners.sort(key=lambda p: p[0])
            
            top_left = top_corners[0]
            top_right = top_corners[1]
            bottom_left = bottom_corners[0]
            bottom_right = bottom_corners[1]
            
            # Berechne einfache Bounding Box des aufgespannten Bereichs
            x_min = min(top_left[0], bottom_left[0])
            x_max = max(top_right[0], bottom_right[0])
            y_min = min(top_left[1], top_right[1])
            y_max = max(bottom_left[1], bottom_right[1])
            
            # SPEICHERE als stabiles Erkennungsgebiet (wird nicht mehr verändert)
            self.stable_detection_area = {
                'x_min': x_min,
                'y_min': y_min,
                'x_max': x_max,
                'y_max': y_max,
                'corners': [top_left, top_right, bottom_left, bottom_right],
                'use_mask': False  # VEREINFACHT: Nur Bounding Box
            }
            
            self.detection_area = self.stable_detection_area.copy()
            
            # Berechne Prozent der Bildfläche für Info
            frame_area = width * height
            detection_area = (x_max - x_min) * (y_max - y_min)
            percent = (detection_area / frame_area) * 100
            
            print(f"4 ROTE Eckpunkte erkannt: {percent:.1f}% Erkennungsbereich | Ecken: TL{top_left} TR{top_right} BL{bottom_left} BR{bottom_right}")
            
            # Zeichne erkannte Eckpunkte zur Visualisierung
            for i, corner in enumerate([top_left, top_right, bottom_left, bottom_right]):
                cv2.circle(frame, corner, 10, (0, 255, 255), -1)  # Gelbe Kreise
                cv2.putText(frame, f"{i+1}", (corner[0]-5, corner[1]+5), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 0), 2)
            
            # Zeichne einfaches Rechteck zwischen den Eckpunkten
            cv2.rectangle(frame, (x_min, y_min), (x_max, y_max), (0, 255, 0), 3)
            
            cv2.putText(frame, f"4 ECKPUNKTE ERKANNT ({percent:.1f}% Bild)", (10, 30), 
                       cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 255), 3)
            cv2.putText(frame, f"Erkennungsbereich: {x_max-x_min}x{y_max-y_min} Pixel", (10, 70), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
            
            return True
            
        except Exception as e:
            print(f"Fehler bei Eckpunkt-Erkennung: {e}")
            # Fallback: Vollbild
            height, width = frame.shape[:2]
            self.detection_area = {
                'x_min': 0, 'y_min': 0,
                'x_max': width, 'y_max': height,
                'use_mask': False
            }
            self.detection_mask = None
            return False

    def is_point_in_detection_area(self, x, y):
        """Prüft ob ein Punkt im Erkennungsbereich liegt - VEREINFACHTE Version"""
        try:
            if not hasattr(self, 'detection_area') or self.detection_area is None:
                return True  # Fallback: Akzeptiere alle Punkte
            
            # EINFACHE Bounding Box Prüfung für 4-Eckpunkt-System
            area = self.detection_area
            return (area['x_min'] <= x <= area['x_max'] and 
                    area['y_min'] <= y <= area['y_max'])
                        
        except Exception as e:
            print(f"Fehler bei Punkt-Prüfung: {e}")
            return True  # Fallback: Akzeptiere Punkt

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
            
            # WICHTIG: ERKENNE den projizierten ROTEN Rahmen im Kamerabild (nur einmal für stabiles Rechteck)
            frame_detected = self.detect_beamer_frame_in_image(frame)
            
            if frame_detected:
                # Zeige Info über stabilen Erkennungsbereich
                x_min, y_min = self.detection_area['x_min'], self.detection_area['y_min']
                x_max, y_max = self.detection_area['x_max'], self.detection_area['y_max']
                
                # Grüne Markierung für das stabile Erkennungsrechteck
                cv2.rectangle(frame, (x_min, y_min), (x_max, y_max), (0, 255, 0), 3)
                cv2.putText(frame, "STABILER ERKENNUNGSBEREICH", (x_min + 5, y_min - 10), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
                
                # NEUE FUNKTION: Erkenne Farben vor weißem Hintergrund im Erkennungsbereich
                self.detect_colors_in_detection_area(frame)
                
            else:
                # Fallback-Info
                cv2.putText(frame, "KEINE ECKPUNKTE ERKANNT - VOLLBILD MODUS", (10, 30), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 3)
            
            # DANN Fahrzeuge erkennen (nutzt den definierten Erkennungsbereich)
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
    
    def detect_colors_in_detection_area(self, frame):
        """Erkennt alle Farben vor weißem Hintergrund im Erkennungsbereich und zeigt HSV-Codes an"""
        try:
            if not hasattr(self, 'detection_area') or self.detection_area is None:
                return
                
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            
            # Bereich extrahieren
            x_min, y_min = self.detection_area['x_min'], self.detection_area['y_min']
            x_max, y_max = self.detection_area['x_max'], self.detection_area['y_max']
            
            # ROI (Region of Interest) extrahieren
            roi_hsv = hsv[y_min:y_max, x_min:x_max]
            roi_bgr = frame[y_min:y_max, x_min:x_max]
            
            # Weißen Hintergrund erkennen (hohe Helligkeit, niedrige Sättigung)
            hue, saturation, value = cv2.split(roi_hsv)
            
            # Weiß-Maske: Hohe Helligkeit (> 180) UND niedrige Sättigung (< 50)
            white_mask = np.logical_and(value > 180, saturation < 50)
            
            # Farb-Maske: NICHT weiß UND ausreichend Sättigung für echte Farben
            color_mask = np.logical_and(~white_mask, saturation > 30)
            color_mask = np.logical_and(color_mask, value > 50)  # Nicht zu dunkel
            
            # Entferne rote Bereiche (Eckpunkte)
            red_mask1 = cv2.inRange(roi_hsv, np.array([0, 80, 80]), np.array([10, 255, 255]))
            red_mask2 = cv2.inRange(roi_hsv, np.array([170, 80, 80]), np.array([180, 255, 255]))
            red_combined = np.logical_or(red_mask1 > 0, red_mask2 > 0)
            color_mask = np.logical_and(color_mask, ~red_combined)
            
            # ZUSÄTZLICH: Entferne falsches GELB bei H:95, S:50, V:225 (schlechte Farberkennung)
            false_yellow_mask = cv2.inRange(roi_hsv, np.array([90, 40, 200]), np.array([100, 70, 255]))
            color_mask = np.logical_and(color_mask, false_yellow_mask == 0)
            
            # Finde Farbregionen
            color_mask_uint8 = color_mask.astype(np.uint8) * 255
            
            # Morphologische Operationen für zusammenhängende Regionen
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
            color_mask_uint8 = cv2.morphologyEx(color_mask_uint8, cv2.MORPH_CLOSE, kernel)
            color_mask_uint8 = cv2.morphologyEx(color_mask_uint8, cv2.MORPH_OPEN, kernel)
            
            # Konturen finden
            contours, _ = cv2.findContours(color_mask_uint8, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            detected_colors = []
            
            for contour in contours:
                area = cv2.contourArea(contour)
                if area > 50:  # Mindestgröße
                    # Schwerpunkt der Farbregion
                    M = cv2.moments(contour)
                    if M["m00"] != 0:
                        cx = int(M["m10"] / M["m00"])
                        cy = int(M["m01"] / M["m00"])
                        
                        # HSV-Werte am Schwerpunkt
                        h_val = roi_hsv[cy, cx, 0]
                        s_val = roi_hsv[cy, cx, 1]
                        v_val = roi_hsv[cy, cx, 2]
                        
                        # BGR-Werte für Farbanzeige
                        b_val = roi_bgr[cy, cx, 0]
                        g_val = roi_bgr[cy, cx, 1]
                        r_val = roi_bgr[cy, cx, 2]
                        
                        # Absolute Position im Gesamtbild
                        abs_x = x_min + cx
                        abs_y = y_min + cy
                        
                        detected_colors.append({
                            'position': (abs_x, abs_y),
                            'hsv': (int(h_val), int(s_val), int(v_val)),
                            'bgr': (int(b_val), int(g_val), int(r_val)),
                            'area': area
                        })
            
            # Farben im Bild anzeigen
            y_offset = 80
            for i, color_info in enumerate(detected_colors):
                pos = color_info['position']
                hsv_vals = color_info['hsv']
                bgr_vals = color_info['bgr']
                
                # Markiere Position im Bild
                cv2.circle(frame, pos, 10, (0, 255, 255), 3)  # Gelber Kreis
                cv2.circle(frame, pos, 15, (0, 0, 0), 2)      # Schwarzer Rand
                
                # HSV-Text neben dem Punkt
                hsv_text = f"H:{hsv_vals[0]} S:{hsv_vals[1]} V:{hsv_vals[2]}"
                cv2.putText(frame, hsv_text, (pos[0] + 20, pos[1]), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 2)
                
                # HSV-Werte auch links im Bild auflisten
                color_text = f"Farbe {i+1}: HSV({hsv_vals[0]}, {hsv_vals[1]}, {hsv_vals[2]})"
                cv2.putText(frame, color_text, (10, y_offset + i * 25), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
                
                # Farbbereich-Vorschlag
                h_range = f"H: [{max(0, hsv_vals[0]-10)}, {min(180, hsv_vals[0]+10)}]"
                s_range = f"S: [60, 255]"
                v_range = f"V: [50, 255]"
                cv2.putText(frame, f"    Bereich: {h_range}", (10, y_offset + i * 25 + 12), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 0), 1)
                
            # Anzeige der Gesamtanzahl
            if detected_colors:
                cv2.putText(frame, f"ERKANNTE FARBEN: {len(detected_colors)}", (10, y_offset - 10), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
            else:
                cv2.putText(frame, "KEINE FARBEN ERKANNT", (10, y_offset - 10), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
                           
        except Exception as e:
            print(f"Fehler bei Farberkennung: {e}")

    def draw_all_detected_points(self, frame):
        """Markiert ALLE erkannten Farbpunkte im Bild zur Debugging-Zwecken"""
        try:
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            
            # MAGENTA-Eckpunkte ausschließen aus der Anzeige (Rahmen-Eckpunkte)
            cyan_lower = np.array([85, 100, 100])
            cyan_upper = np.array([105, 255, 255])
            cyan_mask = cv2.inRange(hsv, cyan_lower, cyan_upper)
            
            # Erkenne alle orangen Punkte (Kopffarbe) - ANGEPASST auf echte Orange-Werte
            front_positions = self.find_all_color_centers(hsv, [8, 180, 220], [18, 255, 255], is_front_color=True)
            for pos in front_positions:
                # NUR Punkte INNERHALB des Erkennungsbereichs zeichnen
                cv2.circle(frame, pos, 8, (0, 165, 255), 3)  # Orange für Kopf (dicker)
                cv2.putText(frame, "ORANGE", (pos[0] + 12, pos[1] - 12), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 165, 255), 2)
            
            # Erkenne alle Heckfarben-Punkte (ANGEPASST - Gelb ohne falschen Bereich)
            colors_to_check = [
                ([100, 80, 30], [130, 255, 255], (255, 0, 0), "BLAU"),      # Blau (heller)
                ([40, 60, 50], [80, 255, 255], (0, 255, 0), "GRÜN"),        # Grün (heller)
                ([25, 80, 180], [35, 255, 255], (0, 255, 255), "GELB"),     # Gelb (NICHT H:95 - falsches Gelb ausschließen)
                ([130, 40, 40], [165, 255, 255], (128, 0, 128), "LILA")     # Lila (heller)
            ]
            
            for lower, upper, color, name in colors_to_check:
                positions = self.find_all_color_centers(hsv, lower, upper, is_front_color=False)
                for pos in positions:
                    # NUR Punkte INNERHALB des Erkennungsbereichs zeichnen
                    cv2.circle(frame, pos, 7, color, 3)  # Heckfarben (dicker)
                    cv2.putText(frame, name, (pos[0] + 10, pos[1] - 10), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.4, color, 2)
            
            # Info-Text über Erkennungsbereich
            if hasattr(self, 'detection_area'):
                area_info = f"Erkennungsbereich: {self.detection_area['x_max'] - self.detection_area['x_min']}x{self.detection_area['y_max'] - self.detection_area['y_min']}"
                cv2.putText(frame, area_info, (10, 50), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
                           
        except Exception as e:
            print(f"Fehler beim Zeichnen der erkannten Punkte: {e}")


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

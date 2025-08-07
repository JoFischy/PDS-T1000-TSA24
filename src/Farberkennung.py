import cv2
import numpy as np
import json
import time
import threading

class SimpleCoordinateDetector:
    """
    Einfache Koordinaten-Erkennung die normalisierte Koordinaten 
    relativ zum Crop-Bereich exportiert für C++ Weiterverarbeitung
    """

    def __init__(self):
        self.cap = None
        self.min_size = 80
        
        # Crop-Einstellungen
        self.crop_left = 50
        self.crop_right = 50
        self.crop_top = 50
        self.crop_bottom = 50
        
        # Farbmesser-Einstellungen
        self.color_picker_x = 320  # Mitte des Bildes
        self.color_picker_y = 240
        self.color_picker_enabled = 1  # 1 = aktiviert, 0 = deaktiviert
        
        # Current detection results (for C++ access)
        self.current_detections = []
        self.detection_lock = threading.Lock()
        self.running = False
        
        # Farbdefinitionen (HSV)
        self.color_definitions = {
            'Front': [114, 255, 150],
            'Heck1': [72, 255, 60],
            'Heck2': [178, 190, 210],
            'Heck3': [20, 150, 240],
            'Heck4': [11, 160, 255]
        }
        
        # Toleranz-Einstellungen für jede Farbe
        self.color_tolerances = {
            'Front': 50,
            'Heck1': 50,
            'Heck2': 50,
            'Heck3': 50,
            'Heck4': 50
        }

    def create_trackbars(self):
        """Erstelle Schieberegler"""
        cv2.namedWindow('Einstellungen', cv2.WINDOW_NORMAL)
        cv2.resizeWindow('Einstellungen', 400, 800)
        cv2.moveWindow('Einstellungen', 50, 50)
        
        cv2.createTrackbar('Mindest-Groesse', 'Einstellungen', self.min_size, 300, lambda x: None)
        cv2.createTrackbar('Crop Links', 'Einstellungen', self.crop_left, 400, lambda x: None)
        cv2.createTrackbar('Crop Rechts', 'Einstellungen', self.crop_right, 400, lambda x: None)
        cv2.createTrackbar('Crop Oben', 'Einstellungen', self.crop_top, 300, lambda x: None)
        cv2.createTrackbar('Crop Unten', 'Einstellungen', self.crop_bottom, 300, lambda x: None)
        
        # Farbmesser-Trackbars
        cv2.createTrackbar('Farbmesser EIN/AUS', 'Einstellungen', self.color_picker_enabled, 1, lambda x: None)
        cv2.createTrackbar('Farbmesser X', 'Einstellungen', self.color_picker_x, 640, lambda x: None)
        cv2.createTrackbar('Farbmesser Y', 'Einstellungen', self.color_picker_y, 480, lambda x: None)
        
        # Toleranz-Schieberegler für jede Farbe
        for color_name in self.color_definitions.keys():
            cv2.createTrackbar(f'Toleranz {color_name}', 'Einstellungen', self.color_tolerances[color_name], 200, lambda x: None)

    def get_trackbar_values(self):
        """Lese Trackbar-Werte"""
        self.min_size = cv2.getTrackbarPos('Mindest-Groesse', 'Einstellungen')
        self.crop_left = cv2.getTrackbarPos('Crop Links', 'Einstellungen')
        self.crop_right = cv2.getTrackbarPos('Crop Rechts', 'Einstellungen')
        self.crop_top = cv2.getTrackbarPos('Crop Oben', 'Einstellungen')
        self.crop_bottom = cv2.getTrackbarPos('Crop Unten', 'Einstellungen')
        
        # Farbmesser-Werte lesen
        self.color_picker_enabled = cv2.getTrackbarPos('Farbmesser EIN/AUS', 'Einstellungen')
        self.color_picker_x = cv2.getTrackbarPos('Farbmesser X', 'Einstellungen')
        self.color_picker_y = cv2.getTrackbarPos('Farbmesser Y', 'Einstellungen')
        
        # Toleranz-Werte für jede Farbe lesen
        for color_name in self.color_definitions.keys():
            self.color_tolerances[color_name] = cv2.getTrackbarPos(f'Toleranz {color_name}', 'Einstellungen')

    def initialize_camera(self):
        """Kamera initialisieren"""
        try:
            for camera_index in [0, 2, 3]:
                print(f"Versuche Kamera {camera_index}...")
                self.cap = cv2.VideoCapture(camera_index)
                
                if self.cap.isOpened():
                    ret, frame = self.cap.read()
                    if ret and frame is not None:
                        print(f"OK Kamera bereit (Index: {camera_index})")
                        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
                        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
                        return True
                    else:
                        self.cap.release()
            return False
        except Exception as e:
            print(f"Kamera-Fehler: {e}")
            return False

    def crop_frame(self, frame):
        """Bild beschneiden"""
        height, width = frame.shape[:2]
        
        left = max(0, self.crop_left)
        right = max(left + 50, width - self.crop_right)
        top = max(0, self.crop_top)
        bottom = max(top + 50, height - self.crop_bottom)
        
        cropped_frame = frame[top:bottom, left:right]
        
        return cropped_frame, (left, top, right, bottom)

    def find_closest_color(self, hsv_value):
        """Finde wahrscheinlichste Farbe mit starker Gewichtung auf H-Wert und zirkulärer H-Logik"""
        h, s, v = hsv_value
        closest_color = None
        min_distance = float('inf')
        
        for color_name, color_hsv in self.color_definitions.items():
            tolerance = self.color_tolerances[color_name]
            
            # H-Wert Distanz (zirkulär für Rot-Bereich)
            # OpenCV: H = 0-179, aber Rot kann bei 0 UND 179 liegen!
            dh1 = abs(h - color_hsv[0])
            dh2 = 180 - dh1  # Alternative über den "Rückweg"
            dh = min(dh1, dh2)  # Kürzeste zirkuläre Distanz
            
            ds = abs(s - color_hsv[1])
            dv = abs(v - color_hsv[2])
            
            # EXTREM gewichtete Distanz - H-Wert ist 10x wichtiger!
            # H dominiert komplett die Farbentscheidung, S und V sind fast irrelevant
            distance = (dh * 10) + (ds * 0.3) + (dv * 0.1)
            
            # Noch strengere H-Toleranz für präzise Farberkennung
            if color_hsv[0] <= 20 or color_hsv[0] >= 160:  # Rot-Bereich
                h_tolerance = min(20, tolerance * 0.4)  # Größere Toleranz für Rot
            else:
                h_tolerance = min(10, tolerance * 0.2)  # Sehr strenge Toleranz für andere Farben
            
            if dh <= h_tolerance and distance < min_distance:
                min_distance = distance
                closest_color = color_name
                
        return closest_color, min_distance

    def normalize_coordinates(self, pos, crop_width, crop_height):
        """
        Normalisiere Koordinaten relativ zum Crop-Bereich
        (0,0) = oben links, (crop_width, crop_height) = unten rechts
        """
        norm_x = float(pos[0])
        norm_y = float(pos[1])
        
        # In Grenzen halten
        norm_x = max(0.0, min(norm_x, float(crop_width)))
        norm_y = max(0.0, min(norm_y, float(crop_height)))
        
        return (round(norm_x, 2), round(norm_y, 2))

    def measure_color_at_position(self, frame, hsv_frame):
        """Messe Farbe an der gewählten Position und gib RGB/HSV-Werte zurück"""
        if not self.color_picker_enabled:
            return None, None, None
        
        height, width = frame.shape[:2]
        
        # Koordinaten in Bildgrenzen halten
        x = max(0, min(self.color_picker_x, width - 1))
        y = max(0, min(self.color_picker_y, height - 1))
        
        # BGR (OpenCV) und HSV Werte am Pixel lesen
        bgr_pixel = frame[y, x]
        hsv_pixel = hsv_frame[y, x]
        
        # BGR zu RGB konvertieren für bessere Verständlichkeit
        rgb_pixel = (int(bgr_pixel[2]), int(bgr_pixel[1]), int(bgr_pixel[0]))
        hsv_values = (int(hsv_pixel[0]), int(hsv_pixel[1]), int(hsv_pixel[2]))
        
        return (x, y), rgb_pixel, hsv_values

    def draw_color_picker(self, frame, position, rgb_values, hsv_values):
        """Zeichne Farbmesser-Visualisierung"""
        if position is None:
            return
        
        x, y = position
        
        # Großes Fadenkreuz zeichnen
        cv2.line(frame, (x-20, y), (x+20, y), (0, 255, 255), 2)
        cv2.line(frame, (x, y-20), (x, y+20), (0, 255, 255), 2)
        cv2.circle(frame, (x, y), 25, (0, 255, 255), 2)
        cv2.circle(frame, (x, y), 3, (0, 0, 255), -1)
        
        # Textbox für Werte
        text_x = x + 30
        text_y = y - 40
        
        # Hintergrund für bessere Lesbarkeit
        cv2.rectangle(frame, (text_x-5, text_y-50), (text_x+280, text_y+20), (0, 0, 0), -1)
        cv2.rectangle(frame, (text_x-5, text_y-50), (text_x+280, text_y+20), (0, 255, 255), 1)
        
        # RGB Werte anzeigen
        rgb_text = f"RGB: {rgb_values[0]}, {rgb_values[1]}, {rgb_values[2]}"
        cv2.putText(frame, rgb_text, (text_x, text_y-30), 
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        
        # HSV Werte anzeigen
        hsv_text = f"HSV: {hsv_values[0]}, {hsv_values[1]}, {hsv_values[2]}"
        cv2.putText(frame, hsv_text, (text_x, text_y-10), 
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        
        # Position anzeigen
        pos_text = f"Pos: ({x}, {y})"
        cv2.putText(frame, pos_text, (text_x, text_y+10), 
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

    def detect_colors(self, cropped_frame):
        """Erkenne Farben und gib normalisierte Koordinaten zurück"""
        detected_objects = []
        
        try:
            hsv = cv2.cvtColor(cropped_frame, cv2.COLOR_BGR2HSV)
            
            # Maske für Nicht-Weiß
            lower_nonwhite = np.array([0, 30, 30])
            upper_nonwhite = np.array([179, 255, 255])
            nonwhite_mask = cv2.inRange(hsv, lower_nonwhite, upper_nonwhite)
            
            # Morphologie
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
            nonwhite_mask = cv2.morphologyEx(nonwhite_mask, cv2.MORPH_OPEN, kernel)
            nonwhite_mask = cv2.morphologyEx(nonwhite_mask, cv2.MORPH_CLOSE, kernel)
            
            # Konturen finden
            contours, _ = cv2.findContours(nonwhite_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            crop_height, crop_width = cropped_frame.shape[:2]
            all_candidates = []
            
            for i, contour in enumerate(contours):
                area = cv2.contourArea(contour)
                if area > self.min_size:
                    M = cv2.moments(contour)
                    if M["m00"] != 0:
                        cx = int(M["m10"] / M["m00"])
                        cy = int(M["m01"] / M["m00"])
                        
                        if 0 <= cy < hsv.shape[0] and 0 <= cx < hsv.shape[1]:
                            hsv_pixel = hsv[cy, cx]
                            h, s, v = hsv_pixel
                            
                            closest_color, distance = self.find_closest_color((int(h), int(s), int(v)))
                            normalized_coords = self.normalize_coordinates((cx, cy), crop_width, crop_height)
                            
                            all_candidates.append({
                                'id': i + 1,
                                'position_px': (cx, cy),
                                'normalized_coords': normalized_coords,
                                'classified_color': closest_color,
                                'color_distance': distance,
                                'area': area
                            })
            
            # Beste Matches pro Farbe filtern - MAXIMAL 4 Front und je 1 Heck1-4
            heck_colors = ['Heck1', 'Heck2', 'Heck3', 'Heck4']
            used_heck_colors = set()
            front_count = 0
            max_front_points = 4
            
            all_candidates.sort(key=lambda x: x['color_distance'])
            
            for candidate in all_candidates:
                color = candidate['classified_color']
                
                if color == 'Front' and front_count < max_front_points:
                    detected_objects.append(candidate)
                    front_count += 1
                elif color in heck_colors and color not in used_heck_colors:
                    detected_objects.append(candidate)
                    used_heck_colors.add(color)
            
            return detected_objects, crop_width, crop_height
            
        except Exception as e:
            print(f"Erkennungsfehler: {e}")
            return [], 0, 0

    def save_coordinates_for_cpp(self, detected_objects, crop_width, crop_height):
        """Speichere Koordinaten für C++ Verarbeitung"""
        try:
            output_data = {
                'timestamp': time.time(),
                'crop_area': {
                    'width': crop_width,
                    'height': crop_height
                },
                'objects': []
            }
            
            for obj in detected_objects:
                output_data['objects'].append({
                    'id': obj['id'],
                    'color': obj['classified_color'],
                    'coordinates': {
                        'x': obj['normalized_coords'][0],
                        'y': obj['normalized_coords'][1]
                    },
                    'area': obj['area']
                })
            
            # Speichere in JSON-Datei für C++
            with open('coordinates.json', 'w') as f:
                json.dump(output_data, f, indent=2)
                
            return True
            
        except Exception as e:
            print(f"Speicher-Fehler: {e}")
            return False

    def run_detection(self):
        """Hauptschleife"""
        if not self.initialize_camera():
            print("FEHLER: Kamera konnte nicht initialisiert werden!")
            return
        
        self.create_trackbars()
        
        print("=== EINFACHE KOORDINATEN-ERKENNUNG ===")
        print("Koordinaten werden für C++ normalisiert (0,0 = oben links)")
        print("ESC = Beenden")
        print("=====================================")
        
        while True:
            ret, frame = self.cap.read()
            if not ret:
                break
            
            self.get_trackbar_values()
            cropped_frame, crop_bounds = self.crop_frame(frame)
            detected_objects, crop_width, crop_height = self.detect_colors(cropped_frame)
            
            # Farbmessung für das gesamte Frame (nicht nur crop)
            hsv_full_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            color_position, rgb_values, hsv_values = self.measure_color_at_position(frame, hsv_full_frame)
            
            # Speichere Koordinaten für C++
            if detected_objects:
                if self.save_coordinates_for_cpp(detected_objects, crop_width, crop_height):
                    print(f"OK {len(detected_objects)} Objekte erkannt und gespeichert")
                    for obj in detected_objects:
                        coords = obj['normalized_coords']
                        print(f"  {obj['classified_color']}: ({coords[0]}, {coords[1]})")
            
            # Visualisierung
            left, top, right, bottom = crop_bounds
            cv2.rectangle(frame, (left, top), (right, bottom), (0, 255, 0), 3)
            cv2.putText(frame, "ERKENNUNGSBEREICH", (left, top - 10), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            
            # Zeichne erkannte Objekte
            for obj in detected_objects:
                pos_cropped = obj['position_px']
                pos_original = (pos_cropped[0] + left, pos_cropped[1] + top)
                coords = obj['normalized_coords']
                
                cv2.circle(frame, pos_original, 8, (0, 255, 255), 3)
                
                info_text = f"{obj['classified_color']}: ({coords[0]},{coords[1]})"
                cv2.putText(frame, info_text, (pos_original[0] + 12, pos_original[1] - 12), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 2)
            
            # Status
            cv2.putText(frame, f"OBJEKTE: {len(detected_objects)}", 
                       (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
            cv2.putText(frame, f"Crop-Bereich: {crop_width}x{crop_height}", 
                       (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2)
            
            # Toleranz-Anzeige
            y_offset = 90
            for color_name, tolerance in self.color_tolerances.items():
                cv2.putText(frame, f"{color_name}: Tol={tolerance}", 
                           (10, y_offset), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 0), 1)
                y_offset += 20
            
            # Farbmesser zeichnen (falls aktiviert)
            if self.color_picker_enabled and color_position:
                self.draw_color_picker(frame, color_position, rgb_values, hsv_values)
                
                # Zeige auch in der Konsole (alle 30 Frames um Spam zu vermeiden)
                frame_count = getattr(self, '_frame_count', 0)
                self._frame_count = frame_count + 1
                if self._frame_count % 30 == 0:
                    print(f"Farbmesser bei ({color_position[0]}, {color_position[1]}): RGB{rgb_values} HSV{hsv_values}")
            
            # Zeige Video mit erkannten Objekten
            cv2.namedWindow("Koordinaten-Erkennung", cv2.WINDOW_NORMAL)
            cv2.namedWindow("Crop-Bereich", cv2.WINDOW_NORMAL)
            cv2.resizeWindow("Koordinaten-Erkennung", 640, 480)
            cv2.resizeWindow("Crop-Bereich", 640, 480)
            cv2.moveWindow("Koordinaten-Erkennung", 100, 100)
            cv2.moveWindow("Crop-Bereich", 800, 100)
            
            cv2.imshow("Koordinaten-Erkennung", frame)
            cv2.imshow("Crop-Bereich", cropped_frame)
            
            # Force window update
            cv2.waitKey(1)
            
            if cv2.waitKey(30) & 0xFF == 27:  # ESC
                break
        
        self.cap.release()
        cv2.destroyAllWindows()
        print("Koordinaten-Erkennung beendet.")

    def get_current_detections(self):
        """Aktuelle Erkennungen für C++ zurückgeben"""
        with self.detection_lock:
            return self.current_detections.copy()

    def process_single_frame(self):
        """Verarbeite einen einzelnen Frame und gib Erkennungen zurück (für C++ ohne Threading)"""
        if self.cap is None:
            return []
        
        ret, frame = self.cap.read()
        if not ret:
            return []
        
        self.get_trackbar_values()
        cropped_frame, crop_bounds = self.crop_frame(frame)
        detected_objects, crop_width, crop_height = self.detect_colors(cropped_frame)
        
        # Konvertiere für C++ Format
        cpp_objects = []
        for obj in detected_objects:
            cpp_obj = {
                'id': obj['id'],
                'classified_color': obj['classified_color'],
                'normalized_coords': obj['normalized_coords'],
                'area': obj['area'],
                'crop_width': crop_width,
                'crop_height': crop_height
            }
            cpp_objects.append(cpp_obj)
        
        return cpp_objects

    def process_frame_with_display(self):
        """Verarbeite einen Frame und zeige Ergebnisse an"""
        if self.cap is None:
            return []
        
        ret, frame = self.cap.read()
        if not ret:
            return []
        
        self.get_trackbar_values()
        cropped_frame, crop_bounds = self.crop_frame(frame)
        detected_objects, crop_width, crop_height = self.detect_colors(cropped_frame)
        
        # Farbmessung für das gesamte Frame
        hsv_full_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        color_position, rgb_values, hsv_values = self.measure_color_at_position(frame, hsv_full_frame)
        
        # Speichere Koordinaten für eventuelle JSON-Ausgabe
        if detected_objects:
            self.save_coordinates_for_cpp(detected_objects, crop_width, crop_height)
        
        # Visualisierung für Debug
        left, top, right, bottom = crop_bounds
        cv2.rectangle(frame, (left, top), (right, bottom), (0, 255, 0), 3)
        cv2.putText(frame, "ERKENNUNGSBEREICH", (left, top - 10), 
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        
        # Zeichne erkannte Objekte
        for obj in detected_objects:
            pos_cropped = obj['position_px']
            pos_original = (pos_cropped[0] + left, pos_cropped[1] + top)
            coords = obj['normalized_coords']
            
            cv2.circle(frame, pos_original, 8, (0, 255, 255), 3)
            
            info_text = f"{obj['classified_color']}: ({coords[0]},{coords[1]})"
            cv2.putText(frame, info_text, (pos_original[0] + 12, pos_original[1] - 12), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 2)
        
        # Status anzeigen
        cv2.putText(frame, f"OBJEKTE: {len(detected_objects)}", 
                   (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        cv2.putText(frame, f"Crop-Bereich: {crop_width}x{crop_height}", 
                   (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2)
        
        # Farbmesser zeichnen (falls aktiviert)
        if self.color_picker_enabled and color_position:
            self.draw_color_picker(frame, color_position, rgb_values, hsv_values)
        
        # Zeige Fenster
        cv2.namedWindow("Koordinaten-Erkennung", cv2.WINDOW_NORMAL)
        cv2.namedWindow("Crop-Bereich", cv2.WINDOW_NORMAL)
        cv2.resizeWindow("Koordinaten-Erkennung", 640, 480)
        cv2.resizeWindow("Crop-Bereich", 640, 480)
        cv2.moveWindow("Koordinaten-Erkennung", 100, 100)
        cv2.moveWindow("Crop-Bereich", 800, 100)
        
        cv2.imshow("Koordinaten-Erkennung", frame)
        cv2.imshow("Crop-Bereich", cropped_frame)
        cv2.waitKey(1)  # Non-blocking
        
        # Konvertiere für C++ Format
        cpp_objects = []
        for obj in detected_objects:
            cpp_obj = {
                'id': obj['id'],
                'classified_color': obj['classified_color'],
                'normalized_coords': obj['normalized_coords'],
                'area': obj['area'],
                'crop_width': crop_width,
                'crop_height': crop_height
            }
            cpp_objects.append(cpp_obj)
        
        return cpp_objects

    def cleanup(self):
        """Aufräumen"""
        if self.cap:
            self.cap.release()
        cv2.destroyAllWindows()
        print("Python Koordinaten-Detektor aufgeräumt.")

# Hauptfunktion für direkten Start
if __name__ == "__main__":
    detector = SimpleCoordinateDetector()
    detector.run_detection()

# === GLOBALE FUNKTIONEN FÜR C++ INTEGRATION ===
_global_detector = None

def initialize_detector():
    """Initialisiere den Detektor (von C++ aufgerufen)"""
    global _global_detector
    try:
        _global_detector = SimpleCoordinateDetector()
        success = _global_detector.initialize_camera()
        if success:
            _global_detector.create_trackbars()
            print("Kamera und Farberkennung initialisiert")
            return True
        else:
            print("Fehler: Kamera konnte nicht initialisiert werden")
            return False
    except Exception as e:
        print(f"Fehler bei Initialisierung: {e}")
        return False

def detect_objects():
    """Erkenne Objekte und gib sie als Liste zurück (von C++ aufgerufen)"""
    global _global_detector
    if _global_detector is None:
        print("Detektor nicht initialisiert!")
        return []
    
    try:
        return _global_detector.process_frame_with_display()
    except Exception as e:
        print(f"Fehler bei Objekterkennung: {e}")
        return []

def cleanup_detector():
    """Räume den Detektor auf (von C++ aufgerufen)"""
    global _global_detector
    if _global_detector:
        _global_detector.cleanup()
        _global_detector = None
        print("Detektor aufgeräumt")

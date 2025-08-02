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
        
        # Current detection results (for C++ access)
        self.current_detections = []
        self.detection_lock = threading.Lock()
        self.running = False
        
        # Farbdefinitionen (HSV)
        self.color_definitions = {
            'Front': [167, 79, 183],
            'Heck1': [94, 65, 169],
            'Heck2': [4, 89, 203],
            'Heck3': [33, 41, 187],
            'Heck4': [74, 49, 160]
        }

    def create_trackbars(self):
        """Erstelle Schieberegler"""
        cv2.namedWindow('Einstellungen', cv2.WINDOW_AUTOSIZE)
        cv2.moveWindow('Einstellungen', 50, 50)
        
        cv2.createTrackbar('Mindest-Groesse', 'Einstellungen', self.min_size, 300, lambda x: None)
        cv2.createTrackbar('Crop Links', 'Einstellungen', self.crop_left, 400, lambda x: None)
        cv2.createTrackbar('Crop Rechts', 'Einstellungen', self.crop_right, 400, lambda x: None)
        cv2.createTrackbar('Crop Oben', 'Einstellungen', self.crop_top, 300, lambda x: None)
        cv2.createTrackbar('Crop Unten', 'Einstellungen', self.crop_bottom, 300, lambda x: None)

    def get_trackbar_values(self):
        """Lese Trackbar-Werte"""
        self.min_size = cv2.getTrackbarPos('Mindest-Groesse', 'Einstellungen')
        self.crop_left = cv2.getTrackbarPos('Crop Links', 'Einstellungen')
        self.crop_right = cv2.getTrackbarPos('Crop Rechts', 'Einstellungen')
        self.crop_top = cv2.getTrackbarPos('Crop Oben', 'Einstellungen')
        self.crop_bottom = cv2.getTrackbarPos('Crop Unten', 'Einstellungen')

    def initialize_camera(self):
        """Kamera initialisieren"""
        try:
            for camera_index in [0, 1, 2]:
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
        """Finde wahrscheinlichste Farbe"""
        h, s, v = hsv_value
        closest_color = None
        min_distance = float('inf')
        
        for color_name, color_hsv in self.color_definitions.items():
            dh = min(abs(h - color_hsv[0]), 360 - abs(h - color_hsv[0]))
            ds = abs(s - color_hsv[1])
            dv = abs(v - color_hsv[2])
            
            distance = (dh * 2) + ds + dv
            
            if distance < min_distance:
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
            
            # Beste Matches pro Farbe filtern
            heck_colors = ['Heck1', 'Heck2', 'Heck3', 'Heck4']
            used_heck_colors = set()
            
            all_candidates.sort(key=lambda x: x['color_distance'])
            
            for candidate in all_candidates:
                color = candidate['classified_color']
                
                if color == 'Front':
                    detected_objects.append(candidate)
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
            
            # Zeige Video mit erkannten Objekten
            cv2.namedWindow("Koordinaten-Erkennung", cv2.WINDOW_AUTOSIZE)
            cv2.namedWindow("Crop-Bereich", cv2.WINDOW_AUTOSIZE)
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
        
        cropped_frame, crop_bounds = self.crop_frame(frame)
        detected_objects, crop_width, crop_height = self.detect_colors(cropped_frame)
        
        # Update current detections thread-safe
        with self.detection_lock:
            self.current_detections = detected_objects
        
        return detected_objects

    def process_frame_with_display(self):
        """Verarbeite Frame mit OpenCV-Anzeige für C++ (wie im Interactive Mode)"""
        if self.cap is None:
            return []
        
        ret, frame = self.cap.read()
        if not ret:
            return []
        
        self.get_trackbar_values()
        cropped_frame, crop_bounds = self.crop_frame(frame)
        detected_objects, crop_width, crop_height = self.detect_colors(cropped_frame)
        
        # Füge Crop-Dimensionen zu jedem Objekt hinzu
        for obj in detected_objects:
            obj['crop_width'] = crop_width
            obj['crop_height'] = crop_height
        
        # Visualisierung (wie in run_detection)
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
        
        # Zeige Video mit erkannten Objekten
        cv2.namedWindow("Koordinaten-Erkennung", cv2.WINDOW_AUTOSIZE)
        cv2.namedWindow("Crop-Bereich", cv2.WINDOW_AUTOSIZE)
        cv2.moveWindow("Koordinaten-Erkennung", 100, 100)
        cv2.moveWindow("Crop-Bereich", 800, 100)
        
        cv2.imshow("Koordinaten-Erkennung", frame)
        cv2.imshow("Crop-Bereich", cropped_frame)
        
        # Force window update
        cv2.waitKey(1)
        
        # Update current detections thread-safe
        with self.detection_lock:
            self.current_detections = detected_objects
        
        return detected_objects

    def run_detection_background(self):
        """Erkennungsschleife im Hintergrund (für C++ Threading)"""
        if not self.initialize_camera():
            print("FEHLER: Kamera konnte nicht initialisiert werden!")
            return
        
        self.running = True
        print("=== HINTERGRUND KOORDINATEN-ERKENNUNG ===")
        print("Koordinaten werden für C++ bereitgestellt")
        print("=========================================")
        
        while self.running:
            ret, frame = self.cap.read()
            if not ret:
                break
            
            cropped_frame, crop_bounds = self.crop_frame(frame)
            detected_objects, crop_width, crop_height = self.detect_colors(cropped_frame)
            
            # Update current detections thread-safe
            with self.detection_lock:
                self.current_detections = detected_objects
            
            # Short sleep to prevent busy waiting
            time.sleep(0.033)  # ~30 FPS
        
        self.cap.release()
        print("Hintergrund-Erkennung beendet.")

    def stop_detection(self):
        """Stoppe die Hintergrund-Erkennung"""
        self.running = False

    def cleanup(self):
        """Aufräumen"""
        self.stop_detection()
        if self.cap is not None:
            self.cap.release()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    detector = SimpleCoordinateDetector()
    detector.run_detection()
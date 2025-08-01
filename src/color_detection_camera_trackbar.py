import cv2
import numpy as np
import math

class ColorDetectionCameraTrackbar:
    """
    Farberkennung für Beamer-Projektionsfläche mit Schiebereglern
    
    Erkennt:
    - 4 rote Eckpunkte (Orientierung)
    - Alle anderen Farben innerhalb des Bereichs (außer Schwarz/Weiß)
    - Mit einstellbaren Parametern über Trackbars
    """
    
    def __init__(self):
        self.cap = None
        self.detection_area = None
        
        # Standard-Werte
        self.min_red_corner_area = 10
        self.min_color_area = 50
        self.red_hue_tolerance = 10
        self.red_saturation_min = 120
        self.red_value_min = 70
        
    def initialize_camera(self):
        """Initialisiert die Kamera"""
        try:
            # Versuche verschiedene Kamera-Indizes
            for camera_index in [0, 1, 2]:
                print(f"Versuche Kamera-Index {camera_index}...")
                self.cap = cv2.VideoCapture(camera_index)
                
                if self.cap.isOpened():
                    ret, frame = self.cap.read()
                    if ret and frame is not None:
                        print(f"Kamera erfolgreich initialisiert (Index: {camera_index})")
                        
                        # Kamera-Einstellungen
                        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
                        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
                        self.cap.set(cv2.CAP_PROP_FPS, 30)
                        
                        return True
                    else:
                        self.cap.release()
                        self.cap = None
            
            print("Keine funktionierende Kamera gefunden!")
            return False
            
        except Exception as e:
            print(f"Kamera-Initialisierung fehlgeschlagen: {e}")
            return False
    
    def create_trackbars(self):
        """Erstellt das Trackbar-Fenster"""
        cv2.namedWindow('Einstellungen', cv2.WINDOW_NORMAL)
        cv2.resizeWindow('Einstellungen', 400, 300)
        
        # Trackbars erstellen
        cv2.createTrackbar('Min Punktgröße (Rot)', 'Einstellungen', self.min_red_corner_area, 200, self.on_trackbar)
        cv2.createTrackbar('Min Farbgröße', 'Einstellungen', self.min_color_area, 200, self.on_trackbar)
        cv2.createTrackbar('Rot Toleranz', 'Einstellungen', self.red_hue_tolerance, 30, self.on_trackbar)
        cv2.createTrackbar('Rot Sättigung Min', 'Einstellungen', self.red_saturation_min, 255, self.on_trackbar)
        cv2.createTrackbar('Rot Helligkeit Min', 'Einstellungen', self.red_value_min, 255, self.on_trackbar)
        
        # Info-Text im Trackbar-Fenster
        info_img = np.zeros((300, 400, 3), dtype=np.uint8)
        cv2.putText(info_img, 'FARBERKENNUNG EINSTELLUNGEN', (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1)
        cv2.putText(info_img, 'Min Punktgroesse: Rote Eckpunkte', (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 200, 200), 1)
        cv2.putText(info_img, 'Min Farbgroesse: Andere Farben', (10, 80), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 200, 200), 1)
        cv2.putText(info_img, 'Rot Toleranz: Farbbereich', (10, 100), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 200, 200), 1)
        cv2.putText(info_img, 'ESC = Beenden', (10, 250), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1)
        cv2.imshow('Einstellungen', info_img)
    
    def on_trackbar(self, val):
        """Callback für Trackbar-Änderungen"""
        try:
            self.min_red_corner_area = cv2.getTrackbarPos('Min Punktgröße (Rot)', 'Einstellungen')
            self.min_color_area = cv2.getTrackbarPos('Min Farbgröße', 'Einstellungen')
            self.red_hue_tolerance = cv2.getTrackbarPos('Rot Toleranz', 'Einstellungen')
            self.red_saturation_min = cv2.getTrackbarPos('Rot Sättigung Min', 'Einstellungen')
            self.red_value_min = cv2.getTrackbarPos('Rot Helligkeit Min', 'Einstellungen')
        except:
            pass  # Ignore trackbar errors during initialization
    
    def detect_red_corners(self, frame):
        """Erkennt die 4 roten Eckpunkte im Kamerabild"""
        try:
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            
            # ROT in HSV (zwei Bereiche wegen Wrap-Around) - Dynamisch einstellbar
            lower_red1 = np.array([0, self.red_saturation_min, self.red_value_min])
            upper_red1 = np.array([self.red_hue_tolerance, 255, 255])
            lower_red2 = np.array([180-self.red_hue_tolerance, self.red_saturation_min, self.red_value_min])
            upper_red2 = np.array([180, 255, 255])
            
            # Rote Masken kombinieren
            red_mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
            red_mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
            red_mask = cv2.add(red_mask1, red_mask2)
            
            # Morphologische Operationen
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
            red_mask = cv2.morphologyEx(red_mask, cv2.MORPH_OPEN, kernel)
            red_mask = cv2.morphologyEx(red_mask, cv2.MORPH_CLOSE, kernel)
            
            # Konturen finden
            contours, _ = cv2.findContours(red_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            corner_centers = []
            for contour in contours:
                area = cv2.contourArea(contour)
                if area > self.min_red_corner_area:
                    # Schwerpunkt berechnen
                    M = cv2.moments(contour)
                    if M["m00"] != 0:
                        cx = int(M["m10"] / M["m00"])
                        cy = int(M["m01"] / M["m00"])
                        corner_centers.append((cx, cy, area))
            
            # Debug: Zeige alle gefundenen roten Bereiche
            if len(contours) > 0:
                print(f"Min: {self.min_red_corner_area}px | Gefunden: {len(contours)} rote Konturen, davon {len(corner_centers)} groß genug")
                for i, (x, y, area) in enumerate(corner_centers[:8]):  # Max 8 anzeigen
                    print(f"  Punkt {i+1}: Position ({x}, {y}), Größe: {area:.1f}px")
            
            # Sortiere nach Größe und nimm die 4 größten
            corner_centers.sort(key=lambda x: x[2], reverse=True)
            corners = [(x, y) for x, y, area in corner_centers[:4]]
            
            if len(corners) >= 4:
                # Berechne Bounding Box zwischen den Eckpunkten
                x_coords = [p[0] for p in corners]
                y_coords = [p[1] for p in corners]
                
                x_min, x_max = min(x_coords), max(x_coords)
                y_min, y_max = min(y_coords), max(y_coords)
                
                self.detection_area = {
                    'x_min': x_min,
                    'y_min': y_min,
                    'x_max': x_max,
                    'y_max': y_max,
                    'corners': corners
                }
                
                return True, corners
            else:
                print(f"Nur {len(corners)} rote Eckpunkte gefunden! Benötige 4.")
                return False, []
                
        except Exception as e:
            print(f"Fehler bei Eckpunkt-Erkennung: {e}")
            return False, []
    
    def detect_colors_in_area(self, frame):
        """Erkennt alle Farben innerhalb des Erkennungsbereichs"""
        detected_colors = []
        
        if self.detection_area is None:
            return detected_colors
        
        try:
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            
            # Bereich extrahieren
            x_min = self.detection_area['x_min']
            y_min = self.detection_area['y_min']
            x_max = self.detection_area['x_max']
            y_max = self.detection_area['y_max']
            
            # ROI (Region of Interest)
            roi_hsv = hsv[y_min:y_max, x_min:x_max]
            roi_bgr = frame[y_min:y_max, x_min:x_max]
            
            # Filter für echte Farben (nicht Schwarz/Weiß)
            hue, saturation, value = cv2.split(roi_hsv)
            
            # Farb-Maske: Ausreichend Sättigung UND nicht zu dunkel/hell
            color_mask = np.logical_and(saturation > 50, value > 30)  # Nicht zu dunkel
            color_mask = np.logical_and(color_mask, value < 240)      # Nicht zu hell (Weiß)
            
            # Entferne rote Bereiche (Eckpunkte)
            red_mask1 = cv2.inRange(roi_hsv, np.array([0, self.red_saturation_min, self.red_value_min]), np.array([self.red_hue_tolerance, 255, 255]))
            red_mask2 = cv2.inRange(roi_hsv, np.array([180-self.red_hue_tolerance, self.red_saturation_min, self.red_value_min]), np.array([180, 255, 255]))
            red_combined = np.logical_or(red_mask1 > 0, red_mask2 > 0)
            color_mask = np.logical_and(color_mask, ~red_combined)
            
            # Morphologische Operationen
            color_mask_uint8 = color_mask.astype(np.uint8) * 255
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
            color_mask_uint8 = cv2.morphologyEx(color_mask_uint8, cv2.MORPH_OPEN, kernel)
            color_mask_uint8 = cv2.morphologyEx(color_mask_uint8, cv2.MORPH_CLOSE, kernel)
            
            # Konturen finden
            contours, _ = cv2.findContours(color_mask_uint8, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            for contour in contours:
                area = cv2.contourArea(contour)
                if area > self.min_color_area:
                    # Schwerpunkt
                    M = cv2.moments(contour)
                    if M["m00"] != 0:
                        cx = int(M["m10"] / M["m00"])
                        cy = int(M["m01"] / M["m00"])
                        
                        # HSV-Werte am Schwerpunkt
                        h_val = roi_hsv[cy, cx, 0]
                        s_val = roi_hsv[cy, cx, 1]
                        v_val = roi_hsv[cy, cx, 2]
                        
                        # BGR-Werte
                        b_val = roi_bgr[cy, cx, 0]
                        g_val = roi_bgr[cy, cx, 1]
                        r_val = roi_bgr[cy, cx, 2]
                        
                        # Absolute Position
                        abs_x = x_min + cx
                        abs_y = y_min + cy
                        
                        # Farb-Klassifikation
                        color_name = self.classify_color(h_val, s_val, v_val)
                        
                        detected_colors.append({
                            'position': (abs_x, abs_y),
                            'hsv': (int(h_val), int(s_val), int(v_val)),
                            'bgr': (int(b_val), int(g_val), int(r_val)),
                            'area': area,
                            'color_name': color_name
                        })
            
            return detected_colors
            
        except Exception as e:
            print(f"Fehler bei Farberkennung: {e}")
            return []
    
    def classify_color(self, h, s, v):
        """Klassifiziert Farbe basierend auf HSV-Werten"""
        if s < 50:  # Niedrige Sättigung
            return "Grau/Weiß"
        
        if v < 50:  # Niedrige Helligkeit
            return "Dunkel/Schwarz"
        
        # Farbton-basierte Klassifikation
        if h < 15 or h > 170:
            return "Rot"
        elif h < 35:
            return "Orange/Gelb"
        elif h < 85:
            return "Grün"
        elif h < 130:
            return "Blau"
        elif h < 170:
            return "Lila/Magenta"
        else:
            return "Unbekannt"
    
    def run_detection(self):
        """Hauptschleife für Farberkennung mit Trackbars"""
        if not self.initialize_camera():
            print("Kamera konnte nicht initialisiert werden!")
            return
        
        # Trackbars erstellen
        self.create_trackbars()
        
        print("=== FARBERKENNUNG MIT EINSTELLUNGEN GESTARTET ===")
        print("Verwenden Sie die Schieberegler um Parameter anzupassen")
        print("ESC = Beenden")
        print("=================================================")
        
        while True:
            ret, frame = self.cap.read()
            if not ret:
                print("Kein Kamerabild empfangen!")
                break
            
            # Erkenne rote Eckpunkte
            corners_found, corners = self.detect_red_corners(frame)
            
            if corners_found:
                # Zeichne Erkennungsbereich
                area = self.detection_area
                cv2.rectangle(frame, (area['x_min'], area['y_min']), 
                             (area['x_max'], area['y_max']), (0, 255, 0), 2)
                
                # Zeichne Eckpunkte
                for i, corner in enumerate(corners):
                    cv2.circle(frame, corner, 10, (0, 0, 255), -1)
                    cv2.putText(frame, f"{i+1}", (corner[0]-5, corner[1]+5), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
                
                # Erkenne Farben im Bereich
                detected_colors = self.detect_colors_in_area(frame)
                
                # Zeichne erkannte Farben
                for i, color_info in enumerate(detected_colors):
                    pos = color_info['position']
                    hsv_vals = color_info['hsv']
                    color_name = color_info['color_name']
                    
                    # Markiere Position
                    cv2.circle(frame, pos, 8, (0, 255, 255), 3)
                    
                    # HSV-Text
                    hsv_text = f"H:{hsv_vals[0]} S:{hsv_vals[1]} V:{hsv_vals[2]}"
                    cv2.putText(frame, hsv_text, (pos[0] + 15, pos[1] - 10), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 255), 1)
                    
                    # Farb-Name
                    cv2.putText(frame, color_name, (pos[0] + 15, pos[1] + 10), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 0), 1)
                
                # Info-Anzeige mit aktuellen Einstellungen
                cv2.putText(frame, f"ERKENNUNGSBEREICH: {area['x_max']-area['x_min']}x{area['y_max']-area['y_min']} px", 
                           (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                cv2.putText(frame, f"FARBEN ERKANNT: {len(detected_colors)}", 
                           (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
                cv2.putText(frame, f"Min Punktgroesse: {self.min_red_corner_area}px", 
                           (10, 90), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
                
                # HSV-Werte der erkannten Farben auflisten
                y_offset = 120
                for i, color_info in enumerate(detected_colors[:3]):  # Max 3 anzeigen
                    hsv_vals = color_info['hsv']
                    color_text = f"{color_info['color_name']}: HSV({hsv_vals[0]}, {hsv_vals[1]}, {hsv_vals[2]})"
                    cv2.putText(frame, color_text, (10, y_offset + i * 25), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
                
            else:
                cv2.putText(frame, "SUCHE NACH 4 ROTEN ECKPUNKTEN...", (10, 30), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
                cv2.putText(frame, "Positioniere Beamer-Projektion im Kamerabild", (10, 70), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)
                cv2.putText(frame, f"Min Punktgroesse: {self.min_red_corner_area}px", 
                           (10, 110), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
            
            # Anzeigen
            cv2.imshow("Farberkennung - Beamer-Projektionsflaeche", frame)
            
            # ESC zum Beenden
            if cv2.waitKey(1) & 0xFF == 27:  # ESC
                break
        
        # Cleanup
        self.cap.release()
        cv2.destroyAllWindows()
        print("Farberkennung beendet.")

if __name__ == "__main__":
    detector = ColorDetectionCameraTrackbar()
    detector.run_detection()

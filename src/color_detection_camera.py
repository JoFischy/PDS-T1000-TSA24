
import cv2
import numpy as np
import math

class ColorDetectionCamera:
    """
    Farberkennung für Beamer-Projektionsfläche
    
    Erkennt:
    - 4 rote Eckpunkte (Orientierung)
    - Alle anderen Farben innerhalb des Bereichs (außer Schwarz/Weiß)
    - Zeigt HSV-Werte zur Kalibrierung an
    """
    
    def __init__(self):
        self.cap = None
        self.detection_area = None
        
        # Mindestgröße für Farberkennung
        self.MIN_COLOR_AREA = 5  # Muss unter 6 sein für Farben innerhalb des Bereichs
        self.MIN_RED_CORNER_AREA = 7  # Muss unter 8 sein für grünen Punkt
        
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
    
    def detect_red_corners(self, frame):
        """Erkennt die 4 roten Eckpunkte im Kamerabild"""
        try:
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            
            # ROT in HSV (zwei Bereiche wegen Wrap-Around) - Weniger tolerant
            lower_red1 = np.array([0, 120, 70])    # Zurück zu ursprünglichen Werten
            upper_red1 = np.array([10, 255, 255])
            lower_red2 = np.array([170, 120, 70])  # Zurück zu ursprünglichen Werten
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
                if area > self.MIN_RED_CORNER_AREA:
                    # Schwerpunkt berechnen
                    M = cv2.moments(contour)
                    if M["m00"] != 0:
                        cx = int(M["m10"] / M["m00"])
                        cy = int(M["m01"] / M["m00"])
                        corner_centers.append((cx, cy, area))
            
            # Debug: Zeige alle gefundenen roten Bereiche
            if len(contours) > 0:
                print(f"Gefunden: {len(contours)} rote Konturen, davon {len(corner_centers)} groß genug (>{self.MIN_RED_CORNER_AREA} Pixel)")
                for i, (x, y, area) in enumerate(corner_centers):
                    print(f"  Eckpunkt {i+1}: Position ({x}, {y}), Größe: {area:.1f} Pixel")
            
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
            red_mask1 = cv2.inRange(roi_hsv, np.array([0, 120, 70]), np.array([10, 255, 255]))
            red_mask2 = cv2.inRange(roi_hsv, np.array([170, 120, 70]), np.array([180, 255, 255]))
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
                if area > self.MIN_COLOR_AREA:
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
        """Hauptschleife für Farberkennung"""
        if not self.initialize_camera():
            print("Kamera konnte nicht initialisiert werden!")
            return
        
        print("=== FARBERKENNUNG GESTARTET ===")
        print("Suche nach 4 roten Eckpunkten...")
        print("ESC = Beenden")
        print("===============================")
        
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
                
                # Info-Anzeige
                cv2.putText(frame, f"ERKENNUNGSBEREICH: {area['x_max']-area['x_min']}x{area['y_max']-area['y_min']} px", 
                           (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                cv2.putText(frame, f"FARBEN ERKANNT: {len(detected_colors)}", 
                           (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
                
                # HSV-Werte der erkannten Farben auflisten
                y_offset = 90
                for i, color_info in enumerate(detected_colors[:5]):  # Max 5 anzeigen
                    hsv_vals = color_info['hsv']
                    color_text = f"{color_info['color_name']}: HSV({hsv_vals[0]}, {hsv_vals[1]}, {hsv_vals[2]})"
                    cv2.putText(frame, color_text, (10, y_offset + i * 25), 
                               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
                
            else:
                cv2.putText(frame, "SUCHE NACH 4 ROTEN ECKPUNKTEN...", (10, 30), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
                cv2.putText(frame, "Positioniere Beamer-Projektion im Kamerabild", (10, 70), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)
            
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
    detector = ColorDetectionCamera()
    detector.run_detection()

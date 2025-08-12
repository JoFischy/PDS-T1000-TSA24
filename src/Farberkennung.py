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
            'Heck3': [155, 150, 110],
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

        # Fenster-Verwaltung für besseres Verschieben
        self.windows_created = set()
        self.main_windows_positioned = False

        # HSV-Toleranzen für jeden Filter (separate von Gewichtungen)
        self.hsv_tolerances = {
            'Front': {'h': 34, 's': 48, 'v': 255},
            'Heck1': {'h': 9, 's': 0, 'v': 255},
            'Heck2': {'h': 8, 's': 103, 'v': 237},
            'Heck3': {'h': 27, 's': 103, 'v': 41},
            'Heck4': {'h': 6, 's': 117, 'v': 46}
        }

    def create_trackbars(self):
        """Erstelle Schieberegler"""
        if 'Einstellungen' not in self.windows_created:
            cv2.namedWindow('Einstellungen', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Einstellungen', 400, 600)
            cv2.moveWindow('Einstellungen', 50, 50)
            self.windows_created.add('Einstellungen')

        cv2.createTrackbar('Mindest-Groesse', 'Einstellungen', self.min_size, 300, lambda x: None)
        cv2.createTrackbar('Crop Links', 'Einstellungen', self.crop_left, 400, lambda x: None)
        cv2.createTrackbar('Crop Rechts', 'Einstellungen', self.crop_right, 400, lambda x: None)
        cv2.createTrackbar('Crop Oben', 'Einstellungen', self.crop_top, 300, lambda x: None)
        cv2.createTrackbar('Crop Unten', 'Einstellungen', self.crop_bottom, 300, lambda x: None)

        # Farbmesser-Trackbars
        cv2.createTrackbar('Farbmesser EIN/AUS', 'Einstellungen', self.color_picker_enabled, 1, lambda x: None)
        cv2.createTrackbar('Farbmesser X', 'Einstellungen', self.color_picker_x, 640, lambda x: None)
        cv2.createTrackbar('Farbmesser Y', 'Einstellungen', self.color_picker_y, 480, lambda x: None)

        # Erstelle HSV-Schieberegler-Fenster für jede Farbe
        self.create_hsv_trackbars_for_colors()

    def create_hsv_trackbars_for_colors(self):
        """Erstelle HSV-Trackbar-Fenster für jede Farbe"""
        # Positionierung für 5 Fenster (wie vorher)
        window_positions = [
            (50, 700), (350, 700), (650, 700),    # Erste Reihe
            (950, 700), (1250, 700)               # Zweite Reihe
        ]

        pos_index = 0
        for color_name in self.color_definitions.keys():
            window_name = f"HSV-{color_name}"

            # Erstelle verschiebares Fenster nur einmal
            if window_name not in self.windows_created:
                cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
                cv2.resizeWindow(window_name, 280, 150)

                if pos_index < len(window_positions):
                    cv2.moveWindow(window_name, window_positions[pos_index][0], window_positions[pos_index][1])
                    pos_index += 1
                
                self.windows_created.add(window_name)

            # Nur HSV-Toleranz-Trackbars (Gewichtung entfernt)
            cv2.createTrackbar('H-Toleranz', window_name, self.hsv_tolerances[color_name]['h'], 90, lambda x: None)
            cv2.createTrackbar('S-Toleranz', window_name, self.hsv_tolerances[color_name]['s'], 255, lambda x: None)
            cv2.createTrackbar('V-Toleranz', window_name, self.hsv_tolerances[color_name]['v'], 255, lambda x: None)

    def create_or_update_window(self, window_name, width, height, x, y):
        """Erstelle Fenster nur einmal und positioniere es"""
        if window_name not in self.windows_created:
            cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
            cv2.resizeWindow(window_name, width, height)
            cv2.moveWindow(window_name, x, y)
            self.windows_created.add(window_name)
            return True  # Neu erstellt
        return False  # Bereits vorhanden

    def create_hsv_trackbars_for_colors(self):
        """Erstelle HSV-Schieberegler-Fenster für jede Farbe"""
        window_positions = [
            (50, 700), (350, 700), (650, 700),    # Erste Reihe
            (950, 700), (1250, 700)               # Zweite Reihe
        ]

        pos_index = 0
        for color_name in self.color_definitions.keys():
            window_name = f"HSV-{color_name}"

            # Erstelle verschiebares Fenster
            cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
            cv2.resizeWindow(window_name, 280, 150)

            if pos_index < len(window_positions):
                cv2.moveWindow(window_name, window_positions[pos_index][0], window_positions[pos_index][1])
                pos_index += 1

            # Nur HSV-Toleranz-Trackbars (Gewichtung entfernt)
            cv2.createTrackbar('H-Toleranz', window_name, self.hsv_tolerances[color_name]['h'], 90, lambda x: None)
            cv2.createTrackbar('S-Toleranz', window_name, self.hsv_tolerances[color_name]['s'], 255, lambda x: None)
            cv2.createTrackbar('V-Toleranz', window_name, self.hsv_tolerances[color_name]['v'], 255, lambda x: None)

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

        # Nur noch Toleranzen für jede Farbe lesen (Gewichtungen entfernt)
        for color_name in self.color_definitions.keys():
            window_name = f"HSV-{color_name}"

            # Toleranzen lesen
            self.hsv_tolerances[color_name]['h'] = cv2.getTrackbarPos('H-Toleranz', window_name)
            self.hsv_tolerances[color_name]['s'] = cv2.getTrackbarPos('S-Toleranz', window_name)
            self.hsv_tolerances[color_name]['v'] = cv2.getTrackbarPos('V-Toleranz', window_name)

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
        """Finde wahrscheinlichste Farbe mit HSV-Toleranzen"""
        h, s, v = hsv_value
        closest_color = None
        min_distance = float('inf')

        for color_name, color_hsv in self.color_definitions.items():
            # H-Wert Distanz (zirkulär für Rot-Bereich)
            dh1 = abs(h - color_hsv[0])
            dh2 = 180 - dh1  # Alternative über den "Rückweg"
            dh = min(dh1, dh2)  # Kürzeste zirkuläre Distanz

            ds = abs(s - color_hsv[1])
            dv = abs(v - color_hsv[2])

            # Einfache Distanz ohne Gewichtung
            distance = dh + ds + dv

            # Toleranz-Check mit individuellen HSV-Toleranzen
            h_tolerance = self.hsv_tolerances[color_name]['h']
            s_tolerance = self.hsv_tolerances[color_name]['s']
            v_tolerance = self.hsv_tolerances[color_name]['v']

            if (dh <= h_tolerance and 
                ds <= s_tolerance and 
                dv <= v_tolerance and 
                distance < min_distance):
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

    def create_hsv_mask(self, hsv_frame, target_color_name):
        """Erstelle eine HSV-Maske mit individuellen Toleranzen und Gewichtungen"""
        target_hsv = self.color_definitions[target_color_name]
        h_tolerance = self.hsv_tolerances[target_color_name]['h']
        s_tolerance = self.hsv_tolerances[target_color_name]['s']
        v_tolerance = self.hsv_tolerances[target_color_name]['v']

        h_target = target_hsv[0]
        s_target = target_hsv[1]
        v_target = target_hsv[2]

        # Berechne dynamische S und V Grenzen basierend auf Toleranzen
        s_min = max(0, s_target - s_tolerance)
        s_max = min(255, s_target + s_tolerance)
        v_min = max(0, v_target - v_tolerance)
        v_max = min(255, v_target + v_tolerance)

        # Zirkuläre H-Wert Logik (0-179 in OpenCV)
        if h_target - h_tolerance < 0:
            # Wrap around für Rot-Bereich
            lower1 = np.array([0, s_min, v_min], dtype=np.uint8)
            upper1 = np.array([h_target + h_tolerance, s_max, v_max], dtype=np.uint8)
            lower2 = np.array([180 + (h_target - h_tolerance), s_min, v_min], dtype=np.uint8)
            upper2 = np.array([179, s_max, v_max], dtype=np.uint8)

            mask1 = cv2.inRange(hsv_frame, lower1, upper1)
            mask2 = cv2.inRange(hsv_frame, lower2, upper2)
            mask = cv2.bitwise_or(mask1, mask2)
        elif h_target + h_tolerance > 179:
            # Wrap around für Rot-Bereich
            lower1 = np.array([h_target - h_tolerance, s_min, v_min], dtype=np.uint8)
            upper1 = np.array([179, s_max, v_max], dtype=np.uint8)
            lower2 = np.array([0, s_min, v_min], dtype=np.uint8)
            upper2 = np.array([(h_target + h_tolerance) - 180, s_max, v_max], dtype=np.uint8)

            mask1 = cv2.inRange(hsv_frame, lower1, upper1)
            mask2 = cv2.inRange(hsv_frame, lower2, upper2)
            mask = cv2.bitwise_or(mask1, mask2)
        else:
            # Normaler Bereich
            lower = np.array([h_target - h_tolerance, s_min, v_min], dtype=np.uint8)
            upper = np.array([h_target + h_tolerance, s_max, v_max], dtype=np.uint8)
            mask = cv2.inRange(hsv_frame, lower, upper)

        return mask

    def find_highest_density_spot(self, mask):
        """Finde den Spot mit der höchsten Farbdichte - für Front: finde alle separaten Bereiche"""
        # Sanftere Morphologie für Front-Punkte um separate Bereiche zu erhalten
        kernel_small = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel_small)

        # Finde Konturen
        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        if not contours:
            return [], 0

        valid_spots = []

        for contour in contours:
            area = cv2.contourArea(contour)
            if area < self.min_size:
                continue

            # Berechne Dichte-Score: Fläche + Kompaktheit
            perimeter = cv2.arcLength(contour, True)
            if perimeter > 0:
                compactness = (4 * np.pi * area) / (perimeter * perimeter)
                density_score = area * (1 + compactness)  # Belohne kompakte Formen

                # Schwerpunkt berechnen
                M = cv2.moments(contour)
                if M["m00"] != 0:
                    cx = int(M["m10"] / M["m00"])
                    cy = int(M["m01"] / M["m00"])
                    valid_spots.append(((cx, cy), density_score))

        # Sortiere nach Dichte-Score (höchste zuerst)
        valid_spots.sort(key=lambda x: x[1], reverse=True)

        return valid_spots, len(valid_spots)

    def detect_colors(self, cropped_frame):
        """H-Wert-basierte Farberkennung mit Farbdichte-Analyse"""
        detected_objects = []

        try:
            hsv = cv2.cvtColor(cropped_frame, cv2.COLOR_BGR2HSV)
            crop_height, crop_width = cropped_frame.shape[:2]

            # Dictionary für alle Masken (für Anzeige)
            filter_masks = {}

            # Sammle alle Kandidaten mit ihren Dichte-Scores
            all_candidates = []
            candidate_id = 1

            # Für jede definierte Farbe einen HSV-Filter anwenden
            for color_name, color_hsv in self.color_definitions.items():
                # Erstelle HSV-basierte Maske für diese Farbe
                mask = self.create_hsv_mask(hsv, color_name)
                filter_masks[color_name] = mask.copy()  # Speichere für Anzeige

                # Debug: Zeige Anzahl der gefundenen Konturen
                contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
                print(f"Debug {color_name}: {len(contours)} Konturen gefunden")

                # Finde ALLE Bereiche mit Farbdichte (nicht nur den besten)
                spots, spot_count = self.find_highest_density_spot(mask)

                if spots and spot_count > 0:
                    # Für Front: Nimm bis zu 4 beste Spots mit Mindestabstand
                    # Für Heck: Nimm nur den besten Spot
                    if color_name == 'Front':
                        # Für Front-Punkte: stelle sicher, dass sie einen Mindestabstand haben
                        selected_front_spots = []
                        min_distance_between_fronts = 30  # Mindestpixelabstand zwischen Front-Punkten

                        for position, density_score in spots:
                            cx, cy = position

                            # Prüfe Abstand zu bereits ausgewählten Front-Punkten
                            too_close = False
                            for selected_pos, _ in selected_front_spots:
                                distance = np.sqrt((cx - selected_pos[0])**2 + (cy - selected_pos[1])**2)
                                if distance < min_distance_between_fronts:
                                    too_close = True
                                    break

                            if not too_close and len(selected_front_spots) < 4:
                                selected_front_spots.append((position, density_score))

                        # Verarbeite die ausgewählten Front-Spots
                        for i, (position, density_score) in enumerate(selected_front_spots):
                            cx, cy = position
                            normalized_coords = self.normalize_coordinates((cx, cy), crop_width, crop_height)

                            # Validierung: Prüfe tatsächlichen H-Wert am gefundenen Punkt
                            if 0 <= cy < hsv.shape[0] and 0 <= cx < hsv.shape[1]:
                                hsv_pixel = hsv[cy, cx]
                                h_actual = int(hsv_pixel[0])
                                h_target = color_hsv[0]

                                # Zirkuläre H-Distanz berechnen
                                dh1 = abs(h_actual - h_target)
                                dh2 = 180 - dh1
                                h_distance = min(dh1, dh2)

                                # Normale H-Toleranz beibehalten
                                h_tolerance = self.hsv_tolerances[color_name]['h']
                                if h_distance <= h_tolerance:
                                    all_candidates.append({
                                        'id': candidate_id,
                                        'position_px': (cx, cy),
                                        'normalized_coords': normalized_coords,
                                        'classified_color': color_name,
                                        'density_score': density_score,
                                        'h_distance': h_distance,
                                        'area': density_score
                                    })
                                    candidate_id += 1
                                    print(f"Front-Punkt {i+1} erkannt bei ({cx}, {cy}) mit Dichte-Score: {density_score:.1f}")
                    else:
                        # Für Heck-Punkte: nur den besten Spot
                        position, density_score = spots[0]
                        cx, cy = position
                        normalized_coords = self.normalize_coordinates((cx, cy), crop_width, crop_height)

                        # Validierung: Prüfe tatsächlichen H-Wert am gefundenen Punkt
                        if 0 <= cy < hsv.shape[0] and 0 <= cx < hsv.shape[1]:
                            hsv_pixel = hsv[cy, cx]
                            h_actual = int(hsv_pixel[0])
                            h_target = color_hsv[0]

                            # Zirkuläre H-Distanz berechnen
                            dh1 = abs(h_actual - h_target)
                            dh2 = 180 - dh1
                            h_distance = min(dh1, dh2)

                            # Nur akzeptieren wenn H-Wert stimmt
                            h_tolerance = self.hsv_tolerances[color_name]['h']
                            if h_distance <= h_tolerance:
                                all_candidates.append({
                                    'id': candidate_id,
                                    'position_px': (cx, cy),
                                    'normalized_coords': normalized_coords,
                                    'classified_color': color_name,
                                    'density_score': density_score,
                                    'h_distance': h_distance,
                                    'area': density_score
                                })
                                candidate_id += 1
                                print(f"Farbe {color_name} erkannt bei ({cx}, {cy}) mit Dichte-Score: {density_score:.1f}")

            # Filtern: Genau 1 pro Heck-Typ und genau 4 Front-Punkte (immer wahrscheinlichste nehmen)
            heck_colors = ['Heck1', 'Heck2', 'Heck3', 'Heck4']
            used_heck_colors = set()
            front_candidates = []

            # Sortiere nach Dichte-Score (höchste zuerst) - immer wahrscheinlichste nehmen
            all_candidates.sort(key=lambda x: x['density_score'], reverse=True)

            # Sammle Front-Kandidaten und beste Heck-Punkte
            for candidate in all_candidates:
                color = candidate['classified_color']

                if color == 'Front':
                    front_candidates.append(candidate)
                elif color in heck_colors and color not in used_heck_colors:
                    detected_objects.append(candidate)
                    used_heck_colors.add(color)
                    print(f"Heck-Punkt {color} hinzugefügt (Dichte: {candidate['density_score']:.1f})")

            # Nimm immer die 4 wahrscheinlichsten Front-Punkte (auch wenn wenig erkannt)
            front_candidates.sort(key=lambda x: x['density_score'], reverse=True)
            for i, candidate in enumerate(front_candidates[:4]):
                detected_objects.append(candidate)
                print(f"Front-Punkt {i+1} hinzugefügt (Dichte: {candidate['density_score']:.1f})")

            # Falls weniger als 4 Front-Punkte gefunden, trotzdem weitermachen
            if len(front_candidates) < 4:
                print(f"WARNUNG: Nur {len(front_candidates)} Front-Punkte gefunden, sollten 4 sein")

            print(f"Gesamt erkannt: {len(detected_objects)} Objekte (Heck: {len(used_heck_colors)}/4, Front: {min(len(front_candidates), 4)}/4)")
            print(f"Heck-Farben verwendet: {used_heck_colors}")

            # Zeige alle Filtermasken als separate Fenster
            self.display_filter_masks(filter_masks, detected_objects)

            return detected_objects, crop_width, crop_height

        except Exception as e:
            print(f"Erkennungsfehler: {e}")
            return [], 0, 0

    def display_filter_masks(self, filter_masks, detected_objects):
        """Zeige alle Filtermasken als separate verschiebbare Fenster"""
        # Positionierung für 5 Filterfenster (wie vorher)
        window_positions = [
            (1200, 100), (1500, 100), (1800, 100),  # Erste Reihe
            (1200, 350), (1500, 350)                # Zweite Reihe
        ]

        pos_index = 0
        for color_name, mask in filter_masks.items():
            window_name = f"Filter-{color_name}"

            # Erstelle ein farbiges Overlay für bessere Visualisierung
            mask_colored = cv2.applyColorMap(mask, cv2.COLORMAP_HOT)

            # Markiere erkannte Punkte in der Maske
            for obj in detected_objects:
                if obj['classified_color'] == color_name:
                    pos = obj['position_px']
                    cv2.circle(mask_colored, pos, 12, (0, 255, 0), 3)
                    cv2.circle(mask_colored, pos, 4, (255, 255, 255), -1)

            # Erstelle Fenster nur einmal und positioniere es
            if pos_index < len(window_positions):
                x, y = window_positions[pos_index]
                self.create_or_update_window(window_name, 300, 240, x, y)
                pos_index += 1

            # Hole aktuelle HSV-Einstellungen für diese Farbe
            target_hsv = self.color_definitions[color_name]
            h_tol = self.hsv_tolerances[color_name]['h']
            s_tol = self.hsv_tolerances[color_name]['s']
            v_tol = self.hsv_tolerances[color_name]['v']

            # HSV-Info als Text (ohne Gewichtung)
            cv2.putText(mask_colored, f"HSV: {target_hsv[0]},{target_hsv[1]},{target_hsv[2]}", 
                       (5, 15), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
            cv2.putText(mask_colored, f"Toleranz: {h_tol},{s_tol},{v_tol}", 
                       (5, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)

            cv2.imshow(window_name, mask_colored)

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

            # Zeige Video mit erkannten Objekten - Fenster nur einmal erstellen
            if not self.main_windows_positioned:
                self.create_or_update_window("Koordinaten-Erkennung", 640, 480, 100, 100)
                self.create_or_update_window("Crop-Bereich", 640, 480, 800, 100)
                self.main_windows_positioned = True
            
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

        # Zeige Fenster - nur einmal erstellen und positionieren
        if not self.main_windows_positioned:
            self.create_or_update_window("Koordinaten-Erkennung", 640, 480, 100, 100)
            self.create_or_update_window("Crop-Bereich", 640, 480, 800, 100)
            self.main_windows_positioned = True

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

        # Zerstöre alle Fenster inklusive Filtermasken- und HSV-Fenster
        for color_name in self.color_definitions.keys():
            # Filter-Fenster
            filter_window_name = f"Filter-{color_name}"
            try:
                cv2.destroyWindow(filter_window_name)
            except:
                pass

            # HSV-Trackbar-Fenster
            hsv_window_name = f"HSV-{color_name}"
            try:
                cv2.destroyWindow(hsv_window_name)
            except:
                pass

        cv2.destroyAllWindows()
        
        # Zurücksetzen der Fenster-Verwaltung
        self.windows_created.clear()
        self.main_windows_positioned = False
        
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
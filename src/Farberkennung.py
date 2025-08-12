import cv2
import numpy as np
import json
import time
import threading

class SimpleCoordinateDetector:
    """
    Einfache Koordinaten-Erkennung die normalisierte Koordinaten 
    relativ zum Crop-Bereich exportiert für C++ Weiterverarbeitung
    Automatische Fenster-Positionierung auf Monitor 3
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

        # PERFORMANCE MODE: Reduziere Debug-Features für maximale Geschwindigkeit
        self.performance_mode = False  # Kann von C++ aktiviert werden
        self.show_filter_masks = True  # Filtermasken-Anzeige (für HSV-Einstellungen)

        # Fenster-Verwaltung für besseres Verschieben
        self.windows_created = set()
        self.main_windows_positioned = False

        # Monitor 3 Setup für automatische Fensterpositionierung
        self.monitor3_offset_x = 0
        self.monitor3_offset_y = 0
        self.use_monitor3 = False
        self.detect_monitor3_position()

        # Farbdefinitionen (HSV) - FINAL OPTIMIERT
        self.color_definitions = {
            'Front': [114, 255, 150],
            'Heck1': [72, 255, 60],
            'Heck2': [178, 190, 210],
            'Heck3': [155, 150, 110],
            'Heck4': [11, 160, 255]
        }

        # HSV-Toleranzen für jeden Filter - FINAL OPTIMIERT
        self.hsv_tolerances = {
            'Front': {'h': 34, 's': 48, 'v': 255},
            'Heck1': {'h': 9, 's': 0, 'v': 255},
            'Heck2': {'h': 8, 's': 103, 'v': 237},
            'Heck3': {'h': 27, 's': 103, 'v': 41},
            'Heck4': {'h': 6, 's': 117, 'v': 46}
        }

    def detect_monitor3_position(self):
        """Erkenne Position für CV2-Fenster (bevorzugt Monitor 3, wenn verfügbar)"""
        try:
            # Standard: Verwende Monitor 3 Position, wird aber von C++ überschrieben
            # Diese Funktion wird hauptsächlich für Fallback verwendet
            self.monitor3_offset_x = 0
            self.monitor3_offset_y = 0
            self.use_monitor3 = False  # Wird von C++ aktiviert
            print("CV2-Fenster-Positionierung bereit - wartet auf C++ Konfiguration")
            
        except Exception as e:
            print(f"Monitor-Erkennung fehlgeschlagen: {e}")
            self.use_monitor3 = False

    def enable_monitor3_mode(self):
        """Aktiviere CV2-Fenster auf separatem Monitor (nicht Monitor 2 wo Raylib läuft)"""
        self.use_monitor3 = True
        print("CV2-Fenster werden auf separatem Monitor positioniert (nicht auf Raylib-Monitor)")

    def disable_monitor3_mode(self):
        """Deaktiviere separaten Monitor - Standard-Positionen werden verwendet"""
        self.use_monitor3 = False
        print("Separate Monitor-Positionierung deaktiviert - Standard-Positionen werden verwendet")

    def move_existing_windows_to_monitor3(self):
        """Verschiebe alle bereits erstellten Fenster auf Monitor 3"""
        if not self.use_monitor3:
            return
            
        print(f"Verschiebe alle CV2-Fenster auf Monitor 3 (Offset: {self.monitor3_offset_x}, {self.monitor3_offset_y})")
        
        # Liste aller möglichen Fenster
        windows_to_move = [
            "Einstellungen",
            "Koordinaten-Erkennung", 
            "Crop-Bereich"
        ]
        
        # HSV-Fenster hinzufügen
        for color_name in self.color_definitions.keys():
            windows_to_move.append(f"HSV-{color_name}")
            windows_to_move.append(f"Filter-{color_name}")
        
        # Alle Fenster verschieben
        moved_count = 0
        for window_name in windows_to_move:
            try:
                if window_name in self.windows_created or window_name in ["Koordinaten-Erkennung", "Crop-Bereich"]:
                    # Berechne neue Position
                    if window_name == "Einstellungen":
                        new_x = 50 + self.monitor3_offset_x
                        new_y = 50 + self.monitor3_offset_y
                    elif window_name == "Koordinaten-Erkennung":
                        new_x = 100 + self.monitor3_offset_x
                        new_y = 100 + self.monitor3_offset_y
                    elif window_name == "Crop-Bereich":
                        new_x = 800 + self.monitor3_offset_x
                        new_y = 100 + self.monitor3_offset_y
                    elif window_name.startswith("HSV-"):
                        # HSV-Fenster positions
                        color_index = list(self.color_definitions.keys()).index(window_name.replace("HSV-", ""))
                        base_positions = [(50, 700), (350, 700), (650, 700), (950, 700), (1250, 700)]
                        if color_index < len(base_positions):
                            base_x, base_y = base_positions[color_index]
                            new_x = base_x + self.monitor3_offset_x
                            new_y = base_y + self.monitor3_offset_y
                        else:
                            continue
                    elif window_name.startswith("Filter-"):
                        # Filter-Fenster positions
                        color_index = list(self.color_definitions.keys()).index(window_name.replace("Filter-", ""))
                        base_positions = [(50, 400), (350, 400), (650, 400), (950, 400), (1250, 400)]
                        if color_index < len(base_positions):
                            base_x, base_y = base_positions[color_index]
                            new_x = base_x + self.monitor3_offset_x
                            new_y = base_y + self.monitor3_offset_y
                        else:
                            continue
                    else:
                        continue
                    
                    # Fenster verschieben
                    cv2.moveWindow(window_name, new_x, new_y)
                    moved_count += 1
                    
            except Exception as e:
                # Fenster existiert möglicherweise nicht - das ist okay
                pass
                
        print(f"Monitor 3: {moved_count} Fenster erfolgreich verschoben")

    def load_window_positions(self):
        """Lade gespeicherte Fenster-Positionen"""
        try:
            with open(self.window_positions_file, 'r') as f:
                self.saved_positions = json.load(f)
            print(f"Fenster-Positionen geladen: {len(self.saved_positions)} Einträge")
        except FileNotFoundError:
            print("Keine gespeicherten Fenster-Positionen gefunden - verwende Standard-Positionen")
            self.saved_positions = {}
        except Exception as e:
            print(f"Fehler beim Laden der Fenster-Positionen: {e}")
            self.saved_positions = {}

    def get_real_window_positions(self):
        """Hole echte Fenster-Positionen mit verschiedenen Methoden"""
        real_positions = {}
        
        # Methode 1: pygetwindow
        if PYGETWINDOW_AVAILABLE:
            try:
                all_windows = gw.getAllWindows()
                target_windows = [
                    "Einstellungen", "Koordinaten-Erkennung", "Crop-Bereich"
                ]
                for color_name in self.color_definitions.keys():
                    target_windows.append(f"HSV-{color_name}")
                    target_windows.append(f"Filter-{color_name}")
                
                for window in all_windows:
                    if window.title in target_windows:
                        try:
                            real_positions[window.title] = {
                                "x": window.left,
                                "y": window.top,
                                "width": window.width,
                                "height": window.height
                            }
                        except Exception:
                            pass
            except Exception as e:
                print(f"pygetwindow Fehler: {e}")
        
        # Methode 2: Windows API (wenn pygetwindow nicht funktioniert)
        if not real_positions and WINDOWS_API_AVAILABLE:
            try:
                def enum_windows_callback(hwnd, windows_dict):
                    window_title = win32gui.GetWindowText(hwnd)
                    if window_title and any(target in window_title for target in [
                        "Einstellungen", "Koordinaten-Erkennung", "Crop-Bereich", "HSV-", "Filter-"
                    ]):
                        try:
                            rect = win32gui.GetWindowRect(hwnd)
                            windows_dict[window_title] = {
                                "x": rect[0],
                                "y": rect[1], 
                                "width": rect[2] - rect[0],
                                "height": rect[3] - rect[1]
                            }
                        except Exception:
                            pass
                    return True
                
                win32gui.EnumWindows(enum_windows_callback, real_positions)
                
            except Exception as e:
                print(f"Windows API Fehler: {e}")
                
        if real_positions:
            print(f"Echte Fenster-Positionen gefunden: {len(real_positions)} Fenster")
        else:
            print("Keine echten Fenster-Positionen verfügbar - verwende geschätzte Positionen")
            
        return real_positions

    def update_real_positions(self):
        """Periodische Aktualisierung der echten Fenster-Positionen während der Laufzeit"""
        print("LIVE-DEBUG: Starte echte Fenster-Positionserfassung...")
        real_positions = self.get_real_window_positions()
        if real_positions:
            # Aktualisiere die gespeicherten Positionen mit echten Werten
            for window_name, pos_data in real_positions.items():
                if window_name not in self.saved_positions:
                    self.saved_positions[window_name] = {}
                
                self.saved_positions[window_name].update({
                    "x": pos_data["x"],
                    "y": pos_data["y"],
                    "real_position": True,
                    "monitor_offset_x": self.monitor3_offset_x,
                    "monitor_offset_y": self.monitor3_offset_y
                })
            
            print(f"Live-Update: {len(real_positions)} echte Fenster-Positionen erfasst")
        else:
            print("LIVE-DEBUG: Keine echten Fenster-Positionen gefunden")

    def save_window_positions(self):
        """Speichere aktuelle Fenster-Positionen (echte Positionen wenn möglich)"""
        try:
            positions_to_save = {}
            
            # Versuche echte Fenster-Positionen zu holen
            real_positions = self.get_real_window_positions()
            
            if real_positions:
                print("Verwende echte Fenster-Positionen von pygetwindow")
                for window_name, pos_data in real_positions.items():
                    positions_to_save[window_name] = {
                        "x": pos_data["x"],
                        "y": pos_data["y"],
                        "real_position": True
                    }
            else:
                print("Verwende geschätzte Positionen basierend auf Monitor-Setup")
                # Fallback: Verwende berechnete Positionen
                windows_to_save = [
                    "Einstellungen",
                    "Koordinaten-Erkennung", 
                    "Crop-Bereich"
                ]
                
                # HSV-Fenster hinzufügen
                for color_name in self.color_definitions.keys():
                    windows_to_save.append(f"HSV-{color_name}")
                    windows_to_save.append(f"Filter-{color_name}")
                
                # Berechnete Positionen verwenden
                for window_name in windows_to_save:
                    try:
                        if self.use_monitor3:
                            if window_name == "Einstellungen":
                                pos_x = 50 + self.monitor3_offset_x
                                pos_y = 50 + self.monitor3_offset_y
                            elif window_name == "Koordinaten-Erkennung":
                                pos_x = 100 + self.monitor3_offset_x
                                pos_y = 100 + self.monitor3_offset_y
                            elif window_name == "Crop-Bereich":
                                pos_x = 800 + self.monitor3_offset_x
                                pos_y = 100 + self.monitor3_offset_y
                            elif window_name.startswith("HSV-"):
                                color_index = list(self.color_definitions.keys()).index(window_name.replace("HSV-", ""))
                                base_positions = [(50, 700), (350, 700), (650, 700), (950, 700), (1250, 700)]
                                if color_index < len(base_positions):
                                    base_x, base_y = base_positions[color_index]
                                    pos_x = base_x + self.monitor3_offset_x
                                    pos_y = base_y + self.monitor3_offset_y
                                else:
                                    continue
                            elif window_name.startswith("Filter-"):
                                color_index = list(self.color_definitions.keys()).index(window_name.replace("Filter-", ""))
                                base_positions = [(50, 400), (350, 400), (650, 400), (950, 400), (1250, 400)]
                                if color_index < len(base_positions):
                                    base_x, base_y = base_positions[color_index]
                                    pos_x = base_x + self.monitor3_offset_x
                                    pos_y = base_y + self.monitor3_offset_y
                                else:
                                    continue
                            else:
                                continue
                            
                            positions_to_save[window_name] = {
                                "x": pos_x, 
                                "y": pos_y,
                                "monitor_offset_x": self.monitor3_offset_x,
                                "monitor_offset_y": self.monitor3_offset_y,
                                "real_position": False
                            }
                            
                    except Exception:
                        pass
            
            # Speichere Positionen in Datei
            with open(self.window_positions_file, 'w') as f:
                json.dump(positions_to_save, f, indent=2)
            
            print(f"Fenster-Positionen gespeichert: {len(positions_to_save)} Fenster")
            
        except Exception as e:
            print(f"Fehler beim Speichern der Fenster-Positionen: {e}")

    def apply_saved_window_positions(self):
        """Wende gespeicherte Fenster-Positionen an"""
        if not self.saved_positions:
            return
            
        print("Wende gespeicherte Fenster-Positionen an...")
        applied_count = 0
        
        for window_name, pos_data in self.saved_positions.items():
            try:
                if window_name in self.windows_created or window_name in ["Koordinaten-Erkennung", "Crop-Bereich"]:
                    x = pos_data.get("x", 0)
                    y = pos_data.get("y", 0)
                    
                    cv2.moveWindow(window_name, x, y)
                    applied_count += 1
                    
            except Exception:
                pass
                
        print(f"Gespeicherte Positionen angewendet: {applied_count} Fenster")

    def cleanup_and_save(self):
        """Automatisches Speichern beim Programm-Ende (wird von atexit aufgerufen)"""
        try:
            if hasattr(self, 'saved_positions') and hasattr(self, 'window_positions_file'):
                self.save_window_positions()
                print("Fenster-Positionen automatisch beim Beenden gespeichert!")
        except Exception as e:
            print(f"Fehler beim automatischen Speichern: {e}")

    def create_trackbars(self):
        """Erstelle Schieberegler"""
        # Berechne Position für Monitor 3
        settings_x = 50 + (self.monitor3_offset_x if self.use_monitor3 else 0)
        settings_y = 50 + (self.monitor3_offset_y if self.use_monitor3 else 0)
        
        if 'Einstellungen' not in self.windows_created:
            cv2.namedWindow('Einstellungen', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Einstellungen', 400, 600)
            cv2.moveWindow('Einstellungen', settings_x, settings_y)
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
        # Basispositionen für 5 Fenster
        base_positions = [
            (50, 700), (350, 700), (650, 700),    # Erste Reihe
            (950, 700), (1250, 700)               # Zweite Reihe
        ]
        
        # Anpassung für Monitor 3
        window_positions = []
        for base_x, base_y in base_positions:
            adjusted_x = base_x + (self.monitor3_offset_x if self.use_monitor3 else 0)
            adjusted_y = base_y + (self.monitor3_offset_y if self.use_monitor3 else 0)
            window_positions.append((adjusted_x, adjusted_y))

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
        """Erstelle Fenster nur einmal und positioniere es auf Monitor 3"""
        # Anpassung für Monitor 3
        adjusted_x = x + (self.monitor3_offset_x if self.use_monitor3 else 0)
        adjusted_y = y + (self.monitor3_offset_y if self.use_monitor3 else 0)
        
        if window_name not in self.windows_created:
            cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
            cv2.resizeWindow(window_name, width, height)
            cv2.moveWindow(window_name, adjusted_x, adjusted_y)
            self.windows_created.add(window_name)
            return True  # Neu erstellt
        return False  # Bereits vorhanden

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
        """H-Wert-basierte Farberkennung mit Farbdichte-Analyse - OPTIMIERT FÜR GESCHWINDIGKEIT"""
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

                            # SCHNELLE Validierung: Prüfe tatsächlichen H-Wert am gefundenen Punkt
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
                    else:
                        # Für Heck-Punkte: nur den besten Spot
                        position, density_score = spots[0]
                        cx, cy = position
                        normalized_coords = self.normalize_coordinates((cx, cy), crop_width, crop_height)

                        # SCHNELLE Validierung: Prüfe tatsächlichen H-Wert am gefundenen Punkt
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

            # Nimm immer die 4 wahrscheinlichsten Front-Punkte (auch wenn wenig erkannt)
            front_candidates.sort(key=lambda x: x['density_score'], reverse=True)
            for i, candidate in enumerate(front_candidates[:4]):
                detected_objects.append(candidate)

            # Zeige Filtermasken basierend auf Einstellung (für HSV-Justierung)
            if self.show_filter_masks:
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

            # HSV-Info als Text - KOMPAKT
            cv2.putText(mask_colored, f"HSV:{target_hsv[0]},{target_hsv[1]},{target_hsv[2]}", 
                       (5, 15), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
            cv2.putText(mask_colored, f"Tol:{h_tol},{s_tol},{v_tol}", 
                       (5, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)

            cv2.imshow(window_name, mask_colored)

    def save_coordinates_for_cpp(self, detected_objects, crop_width, crop_height):
        """Speichere Koordinaten für C++ Verarbeitung - OPTIMIERT FÜR MAXIMALE GESCHWINDIGKEIT"""
        try:
            output_data = {
                'timestamp': time.time(),
                'crop_area': {'width': crop_width, 'height': crop_height},
                'objects': [],
                'direct_mode': True
            }

            for obj in detected_objects:
                output_data['objects'].append({
                    'id': obj['id'],
                    'color': obj['classified_color'],
                    'coordinates': {
                        'x': obj['normalized_coords'][0],
                        'y': obj['normalized_coords'][1]
                    },
                    'area': obj['area'],
                    'confidence': obj.get('density_score', 1.0)
                })

            # MINIMAL BUFFERING - Sofortiges Schreiben
            with open('coordinates.json', 'w') as f:
                json.dump(output_data, f, separators=(',', ':'))
                f.flush()

            return True
        except:
            return False

    def run_detection(self):
        """Hauptschleife"""
        if not self.initialize_camera():
            print("FEHLER: Kamera konnte nicht initialisiert werden!")
            return

        self.create_trackbars()

        print("=== EINFACHE KOORDINATEN-ERKENNUNG ===")
        print("Koordinaten werden für C++ normalisiert (0,0 = oben links)")
        print("ESC = Beenden, F = Filtermasken ein/aus")
        print("Alle Fenster werden automatisch auf Monitor 3 positioniert")
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

            # Speichere Koordinaten für C++ - SCHNELL ohne Debug-Ausgaben
            if detected_objects:
                if self.save_coordinates_for_cpp(detected_objects, crop_width, crop_height):
                    # Minimale Ausgabe für Performance
                    pass

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

            # Status - MINIMIERT FÜR PERFORMANCE
            cv2.putText(frame, f"OBJEKTE: {len(detected_objects)}", 
                       (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

            # Farbmesser zeichnen (falls aktiviert) - REDUZIERTE DEBUG-AUSGABEN
            if self.color_picker_enabled and color_position:
                self.draw_color_picker(frame, color_position, rgb_values, hsv_values)

                # Reduzierte Konsolen-Ausgabe für bessere Performance (alle 60 Frames)
                frame_count = getattr(self, '_frame_count', 0)
                self._frame_count = frame_count + 1
                if self._frame_count % 60 == 0:
                    print(f"Farbmesser: RGB{rgb_values} HSV{hsv_values}")

            # Zeige Video mit erkannten Objekten - Fenster nur einmal erstellen
            if not self.main_windows_positioned:
                self.create_or_update_window("Koordinaten-Erkennung", 640, 480, 100, 100)
                self.create_or_update_window("Crop-Bereich", 640, 480, 800, 100)
                self.main_windows_positioned = True
            
            cv2.imshow("Koordinaten-Erkennung", frame)
            cv2.imshow("Crop-Bereich", cropped_frame)

            # Force window update
            cv2.waitKey(1)

            # Tastatur-Input verarbeiten
            key = cv2.waitKey(30) & 0xFF
            
            if key == 27:  # ESC
                print("ESC gedrückt - System wird beendet...")
                break
            elif key == ord('f') or key == ord('F'):  # F = Toggle Filtermasken
                self.show_filter_masks = not self.show_filter_masks
                status = "aktiviert" if self.show_filter_masks else "deaktiviert"
                print(f"Filtermasken {status} (F zum Umschalten)")
                
                # Wenn deaktiviert, alle Filterfenster schließen
                if not self.show_filter_masks:
                    for color_name in self.color_definitions.keys():
                        filter_window_name = f"Filter-{color_name}"
                        try:
                            cv2.destroyWindow(filter_window_name)
                        except:
                            pass

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
        # Speichere Fenster-Positionen vor dem Schließen
        self.save_window_positions()
        
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

def enable_performance_mode():
    """Aktiviere Performance-Modus für maximale Geschwindigkeit (von C++ aufgerufen)"""
    global _global_detector
    if _global_detector:
        _global_detector.performance_mode = True
        _global_detector.color_picker_enabled = 0
        _global_detector.show_filter_masks = False
        return True
    return False

def cleanup_detector():
    """Räume den Detektor auf (von C++ aufgerufen)"""
    global _global_detector
    if _global_detector:
        _global_detector.cleanup()
        _global_detector = None
        print("Detektor aufgeräumt")

def enable_monitor3_mode():
    """Aktiviere Monitor 3 Modus für alle CV2-Fenster (von C++ aufgerufen)"""
    global _global_detector
    if _global_detector:
        _global_detector.enable_monitor3_mode()
        return True
    return False

def disable_monitor3_mode():
    """Deaktiviere Monitor 3 Modus (von C++ aufgerufen)"""
    global _global_detector
    if _global_detector:
        _global_detector.disable_monitor3_mode()
        return True
    return False

def set_monitor3_position(offset_x, offset_y):
    """Setze manuelle Monitor 3 Position (von C++ aufgerufen)"""
    global _global_detector
    if _global_detector:
        _global_detector.monitor3_offset_x = int(offset_x)
        _global_detector.monitor3_offset_y = int(offset_y)
        _global_detector.use_monitor3 = True
        
        # WICHTIG: Bereits erstellte Fenster verschieben
        _global_detector.move_existing_windows_to_monitor3()
        
        # Speichere die neuen Positionen
        _global_detector.save_window_positions()
        
        print(f"Monitor 3 Position für CV2-Fenster gesetzt: {offset_x}, {offset_y}")
        print("Alle CV2-Fenster werden ab sofort auf Monitor 3 positioniert")
        return True
    return False
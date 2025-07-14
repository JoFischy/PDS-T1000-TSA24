import cv2
import numpy as np
import math
import time

# Global camera object
cap = None

def init_camera():
    """Initialize the camera"""
    global cap
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Error: Could not open video.")
        return False
    
    # Set camera properties for better performance
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_FPS, 30)
    
    # Warm up the camera
    for i in range(5):
        ret, frame = cap.read()
        if not ret:
            break
    
    return True

def get_car_detection_data():
    """
    Erkennt rote (Heck) und gelbe (Spitze) Objekte und berechnet Autorichtung
    Returns: {
        'red_coords': [x, y] oder None,
        'yellow_coords': [x, y] oder None, 
        'car_angle': float in Grad oder None,
        'distance': float oder None
    }
    """
    global cap
    if cap is None:
        if not init_camera():
            return {
                'red_coords': None,
                'yellow_coords': None,
                'car_angle': None,
                'distance': None
            }
    
    ret, frame = cap.read()
    if not ret:
        return {
            'red_coords': None,
            'yellow_coords': None,
            'car_angle': None,
            'distance': None
        }
    
    # HSV-Konvertierung
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    
    # Rote Farbbereich (Heck)
    lower_red1 = np.array([0, 100, 100])
    upper_red1 = np.array([10, 255, 255])
    lower_red2 = np.array([160, 100, 100])
    upper_red2 = np.array([179, 255, 255])
    
    red_mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
    red_mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
    red_mask = cv2.bitwise_or(red_mask1, red_mask2)
    
    # Gelbe Farbbereich (Spitze)
    lower_yellow = np.array([20, 100, 100])
    upper_yellow = np.array([30, 255, 255])
    yellow_mask = cv2.inRange(hsv, lower_yellow, upper_yellow)
    
    # Rauschfilterung
    kernel = np.ones((5,5), np.uint8)
    red_mask = cv2.morphologyEx(red_mask, cv2.MORPH_OPEN, kernel)
    red_mask = cv2.morphologyEx(red_mask, cv2.MORPH_CLOSE, kernel)
    yellow_mask = cv2.morphologyEx(yellow_mask, cv2.MORPH_OPEN, kernel)
    yellow_mask = cv2.morphologyEx(yellow_mask, cv2.MORPH_CLOSE, kernel)
    
    # Objekterkennung
    red_coords = None
    yellow_coords = None
    
    # Rote Objekte finden
    red_contours, _ = cv2.findContours(red_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if red_contours:
        largest_red = max(red_contours, key=cv2.contourArea)
        if cv2.contourArea(largest_red) > 200:  # Mindestgröße
            M = cv2.moments(largest_red)
            if M["m00"] != 0:
                red_coords = [int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"])]
    
    # Gelbe Objekte finden
    yellow_contours, _ = cv2.findContours(yellow_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if yellow_contours:
        largest_yellow = max(yellow_contours, key=cv2.contourArea)
        if cv2.contourArea(largest_yellow) > 200:  # Mindestgröße
            M = cv2.moments(largest_yellow)
            if M["m00"] != 0:
                yellow_coords = [int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"])]
    
    # Autorichtung berechnen
    car_angle = None
    distance = None
    
    if red_coords and yellow_coords:
        # Vektor von Heck (rot) zur Spitze (gelb)
        dx = yellow_coords[0] - red_coords[0]
        dy = yellow_coords[1] - red_coords[1]
        
        # Winkel in Grad: Gelb oben = 0°, Gelb rechts = 90°
        car_angle = math.degrees(math.atan2(dx, -dy))  # -dy damit oben = 0°
        if car_angle < 0:
            car_angle += 360  # Normalisierung auf 0-360°
            
        # Abstand zwischen den Punkten
        distance = math.sqrt(dx*dx + dy*dy)
    
    return {
        'red_coords': red_coords,
        'yellow_coords': yellow_coords,
        'car_angle': car_angle,
        'distance': distance
    }

def get_car_detection_with_display():
    """
    Wie get_car_detection_data() aber mit Live-Anzeige
    """
    global cap
    if cap is None:
        if not init_camera():
            return {
                'red_coords': None,
                'yellow_coords': None,
                'car_angle': None,
                'distance': None
            }
    
    ret, frame = cap.read()
    if not ret:
        return {
            'red_coords': None,
            'yellow_coords': None,
            'car_angle': None,
            'distance': None
        }
    
    # HSV-Konvertierung
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    
    # Farbmasken wie oben...
    lower_red1 = np.array([0, 100, 100])
    upper_red1 = np.array([10, 255, 255])
    lower_red2 = np.array([160, 100, 100])
    upper_red2 = np.array([179, 255, 255])
    
    red_mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
    red_mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
    red_mask = cv2.bitwise_or(red_mask1, red_mask2)
    
    lower_yellow = np.array([20, 100, 100])
    upper_yellow = np.array([30, 255, 255])
    yellow_mask = cv2.inRange(hsv, lower_yellow, upper_yellow)
    
    kernel = np.ones((5,5), np.uint8)
    red_mask = cv2.morphologyEx(red_mask, cv2.MORPH_OPEN, kernel)
    red_mask = cv2.morphologyEx(red_mask, cv2.MORPH_CLOSE, kernel)
    yellow_mask = cv2.morphologyEx(yellow_mask, cv2.MORPH_OPEN, kernel)
    yellow_mask = cv2.morphologyEx(yellow_mask, cv2.MORPH_CLOSE, kernel)
    
    red_coords = None
    yellow_coords = None
    
    # Objekterkennung und Zeichnen
    red_contours, _ = cv2.findContours(red_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if red_contours:
        largest_red = max(red_contours, key=cv2.contourArea)
        if cv2.contourArea(largest_red) > 200:
            M = cv2.moments(largest_red)
            if M["m00"] != 0:
                red_coords = [int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"])]
                cv2.circle(frame, tuple(red_coords), 15, (0, 0, 255), -1)
                cv2.putText(frame, "HECK", (red_coords[0]-20, red_coords[1]-25), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
    
    yellow_contours, _ = cv2.findContours(yellow_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if yellow_contours:
        largest_yellow = max(yellow_contours, key=cv2.contourArea)
        if cv2.contourArea(largest_yellow) > 200:
            M = cv2.moments(largest_yellow)
            if M["m00"] != 0:
                yellow_coords = [int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"])]
                cv2.circle(frame, tuple(yellow_coords), 15, (0, 255, 255), -1)
                cv2.putText(frame, "SPITZE", (yellow_coords[0]-30, yellow_coords[1]-25), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
    
    # Richtungslinie zeichnen
    car_angle = None
    distance = None
    
    if red_coords and yellow_coords:
        # Linie von Heck zu Spitze
        cv2.line(frame, tuple(red_coords), tuple(yellow_coords), (255, 255, 255), 4)
        
        dx = yellow_coords[0] - red_coords[0]
        dy = yellow_coords[1] - red_coords[1]
        car_angle = math.degrees(math.atan2(dx, -dy))  # -dy damit oben = 0°
        if car_angle < 0:
            car_angle += 360
        distance = math.sqrt(dx*dx + dy*dy)
        
        # Richtungstext
        mid_x = (red_coords[0] + yellow_coords[0]) // 2
        mid_y = (red_coords[1] + yellow_coords[1]) // 2
        cv2.putText(frame, f"{car_angle:.1f}°", (mid_x, mid_y), 
                   cv2.FONT_HERSHEY_SIMPLEX, 1.0, (255, 255, 255), 2)
    
    # Anzeige
    cv2.imshow("Auto Erkennung - Gelb=Spitze, Rot=Heck", frame)
    cv2.waitKey(1)
    
    return {
        'red_coords': red_coords,
        'yellow_coords': yellow_coords,
        'car_angle': car_angle,
        'distance': distance
    }

# Legacy-Funktionen für Kompatibilität
def get_red_object_coordinates():
    """Legacy function - returns red coordinates in old format"""
    data = get_car_detection_data()
    if data['red_coords']:
        x, y = data['red_coords']
        return [(x, y, 30, 30)]  # x, y, w, h Format
    return []

def get_red_object_coordinates_with_display():
    """Legacy function - calls new car detection with display"""
    data = get_car_detection_with_display()
    if data['red_coords']:
        x, y = data['red_coords']
        return [(x, y, 30, 30)]  # x, y, w, h Format
    return []

def cleanup_camera():
    """Release camera resources"""
    global cap
    if cap is not None:
        cap.release()
        cap = None
    cv2.destroyAllWindows()

# Für Testing
if __name__ == "__main__":
    while True:
        data = get_car_detection_with_display()
        print(f"Rot: {data['red_coords']}, Gelb: {data['yellow_coords']}, Winkel: {data['car_angle']}")
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    cleanup_camera()
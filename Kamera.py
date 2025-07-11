import cv2
import numpy as np

cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("Error: Could not open video.")
    exit()

while True:
    ret, frame = cap.read()
    if not ret:
        print("Error: Could not read frame.")
        break

    # Konvertiere das Bild in den HSV-Farbraum
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

     # Definiere den Farbbereich für Rot in HSV
    lower_red1 = np.array([0, 100, 100])
    upper_red1 = np.array([10, 255, 255])
    lower_red2 = np.array([160, 100, 100])
    upper_red2 = np.array([179, 255, 255])

    # Erstelle eine Maske für Rot (zwei Bereiche wegen HSV-Übergang)
    mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
    mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
    mask = cv2.bitwise_or(mask1, mask2)

    # Finde Konturen basierend auf der Maske
    contours, _ = cv2.findContours(mask, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)

    positions = []  # Liste für Positionen
    # Zeichne Rechtecke um die erkannten roten Bereiche
    for contour in contours:
        if cv2.contourArea(contour) > 500:  # Filtere kleine Bereiche aus
            x, y, w, h = cv2.boundingRect(contour)
            positions.append((x, y, w, h))  # Position speichern
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 0, 255), 2)

    # Positionen ausgeben
    if positions:
        print("Gefundene Positionen:", positions)

    # Zeige das Ergebnis
    cv2.imshow('Red Color Detection', frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()


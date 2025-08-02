#pragma once
#include <string>

// 2D Punkt für Koordinaten
struct Point2D {
    float x, y;
    Point2D() : x(0), y(0) {}
    Point2D(float x_, float y_) : x(x_), y(y_) {}
};

// Struktur für ein erkanntes Objekt aus der Farberkennung
struct DetectedObject {
    int id;                     // Objekt-ID
    std::string color;          // Erkannte Farbe (Front, Heck1, Heck2, Heck3, Heck4)
    Point2D coordinates;        // Normalisierte Koordinaten (0-crop_width, 0-crop_height)
    float area;                 // Fläche des erkannten Objekts
    float crop_width;           // Breite des Crop-Bereichs
    float crop_height;          // Höhe des Crop-Bereichs
    
    // Standardkonstruktor
    DetectedObject() : id(0), color(""), coordinates(0, 0), area(0), crop_width(0), crop_height(0) {}
};

// Vereinfachte Datenstruktur für Fahrzeugerkennung (falls noch benötigt)
struct VehicleDetectionData {
    // Hauptposition (Schwerpunkt zwischen Front und Heck)
    Point2D position;           // Hauptkoordinate des Fahrzeugs
    
    // Erkennungsstatus
    bool detected;              // Fahrzeug vollständig erkannt?
    
    // Richtung und Größe
    float angle;                // Richtung in Grad (0° = nach rechts)
    float distance;             // Abstand zwischen Front- und Heckpunkt (Fahrzeuggröße)
    
    // Identifikation
    std::string rear_color;     // Heckfarbe zur Identifikation (rot, blau, grün, lila)
      // Standardkonstruktor
    VehicleDetectionData() : position(0, 0), detected(false), angle(0), distance(0), rear_color("") {}
};

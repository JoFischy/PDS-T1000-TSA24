#pragma once
#include <string>

// 2D Punkt für Koordinaten
struct Point2D {
    float x, y;
    Point2D() : x(0), y(0) {}
    Point2D(float x_, float y_) : x(x_), y(y_) {}
};

// Vereinfachte Datenstruktur für Fahrzeugerkennung
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

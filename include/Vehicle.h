#pragma once
#include <string>

// 2D Punkt für Koordinaten
struct Point2D {
    float x, y;
    Point2D() : x(0), y(0) {}
    Point2D(float x_, float y_) : x(x_), y(y_) {}
};

// Datenstruktur für Fahrzeugerkennung
struct VehicleDetectionData {
    // Koordinaten
    Point2D front_pos;          // Position der vorderen Farbe (Gelb)
    Point2D rear_pos;           // Position der hinteren Farbe (Identifikation)
    
    // Erkennungsstatus
    bool has_front;             // Vordere Farbe erkannt?
    bool has_rear;              // Hintere Farbe erkannt?
    bool has_angle;             // Richtung berechenbar?
    
    // Richtungsdaten
    float angle_degrees;        // Richtung in Grad (0° = nach oben)
    float distance_pixels;      // Abstand zwischen den Farben
    
    // Fahrzeug-Info
    std::string vehicle_name;   // Name des Fahrzeugs (Auto-1, Auto-2, etc.)
    std::string front_color;    // Name der vorderen Farbe (immer "Gelb")
    std::string rear_color;     // Name der hinteren Farbe (Rot, Blau, Grün, Lila)
    
    VehicleDetectionData() : 
        front_pos(0, 0), rear_pos(0, 0),
        has_front(false), has_rear(false), has_angle(false),
        angle_degrees(0), distance_pixels(0),
        vehicle_name(""), front_color("Gelb"), rear_color("") {}
};

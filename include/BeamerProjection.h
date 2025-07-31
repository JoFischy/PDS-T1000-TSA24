#pragma once
#include "raylib.h"
#include "Vehicle.h"
#include <vector>

class BeamerProjection {
private:
    // Kamera-Markierungspunkte (4 MAGENTA Eckpunkte für Kamera-Kalibrierung)
    // Keine Rahmen mehr - nur noch 4 Referenzpunkte in den Ecken
    
    // Warnung-Status
    bool show_warnings;
    std::vector<VehicleDetectionData> vehicle_data;
    
    // Private Hilfsfunktionen
    void draw_camera_frame();           // 4 MAGENTA Eckpunkte für Kamera-Erkennung
    void draw_warning_overlay();
    void draw_vehicles();               // Fahrzeug-Rechtecke
    void draw_vehicle_warnings();
    
public:
    BeamerProjection();
    ~BeamerProjection();
    
    // Initialisierung und Cleanup
    bool initialize(const char* title = "BEAMER PROJEKTION - Boden-Warnungen");
    void cleanup();
    
    // Update und Rendering
    void update(const std::vector<VehicleDetectionData>& fleet_data);
    void draw();
    
    // Warnung-Management
    void set_warning_mode(bool enabled);
    bool is_window_ready() const;
    bool should_close() const;
};

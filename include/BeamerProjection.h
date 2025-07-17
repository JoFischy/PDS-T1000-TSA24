#pragma once
#include "raylib.h"
#include "Vehicle.h"
#include <vector>

class BeamerProjection {
private:
    // Projektor-Fenster Eigenschaften
    const int PROJECTOR_WIDTH = 1920;   // Beamer-Auflösung
    const int PROJECTOR_HEIGHT = 1080;
    const int BORDER_WIDTH = 20;        // Rand-Breite
    
    // Warnung-Status
    bool show_warnings;
    std::vector<VehicleDetectionData> vehicle_data;
    
    // Private Hilfsfunktionen
    void draw_border();
    void draw_warning_overlay();
    void draw_vehicles();  // Neue Methode für Fahrzeug-Rechtecke
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

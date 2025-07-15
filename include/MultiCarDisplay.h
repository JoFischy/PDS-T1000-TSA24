#pragma once
#include "raylib.h"
#include "Vehicle.h"
#include <vector>

class MultiCarDisplay {
private:
    std::vector<VehicleDetectionData> vehicle_data;
    
    // UI-Layout
    const int VEHICLE_PANEL_HEIGHT = 140;
    const int MARGIN = 10;
    
    // Hilfsfunktionen
    void draw_vehicle_panel(const VehicleDetectionData& data, int index);
    void draw_compass(const VehicleDetectionData& data, Vector2 center, float radius);
    void draw_status_indicator(const VehicleDetectionData& data, Vector2 position);
    Color get_vehicle_color(int index);
    
public:
    MultiCarDisplay();
    ~MultiCarDisplay();
    
    void update();
    void draw();
};

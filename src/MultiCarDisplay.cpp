#include "../include/MultiCarDisplay.h"
#include "../include/py_runner.h"
#include <cmath>
#include <string>

MultiCarDisplay::MultiCarDisplay() {
}

MultiCarDisplay::~MultiCarDisplay() {
}

void MultiCarDisplay::update() {
    vehicle_data = get_all_vehicle_detections();
}

void MultiCarDisplay::draw() {
    // Titel
    DrawText("FAHRZEUGFLOTTE - 4 AUTOS TRACKING", 20, 20, 24, DARKBLUE);
    DrawText("Jedes Auto: Vordere Farbe + Hintere Identifikationsfarbe", 20, 50, 16, DARKGRAY);
    
    // Zeichne Panel für jedes Fahrzeug (2x2 Grid)
    for (size_t i = 0; i < vehicle_data.size() && i < 4; i++) {
        draw_vehicle_panel(vehicle_data[i], i);
    }
    
    // Gesamtstatus unten
    int detected_count = 0;
    for (const auto& data : vehicle_data) {
        if (data.has_angle) detected_count++;
    }
    
    std::string status_text = "Erkannte Fahrzeuge: " + std::to_string(detected_count) + " / " + std::to_string((int)vehicle_data.size());
    DrawText(status_text.c_str(), 20, GetScreenHeight() - 40, 18, detected_count > 0 ? DARKGREEN : MAROON);
    
    // Anweisungen
    DrawText("ESC = Beenden | Kamera-Feed läuft parallel", 20, GetScreenHeight() - 20, 14, DARKGRAY);
}

void MultiCarDisplay::draw_vehicle_panel(const VehicleDetectionData& data, int index) {
    // Panel-Position berechnen (2x2 Grid)
    int col = index % 2;
    int row = index / 2;
    int panel_width = (GetScreenWidth() - 3 * MARGIN) / 2;
    int panel_x = MARGIN + col * (panel_width + MARGIN);
    int panel_y = 90 + row * (VEHICLE_PANEL_HEIGHT + MARGIN);
    
    Color vehicle_color = get_vehicle_color(index);
    
    // Panel-Hintergrund
    DrawRectangle(panel_x, panel_y, panel_width, VEHICLE_PANEL_HEIGHT, Fade(vehicle_color, 0.1f));
    DrawRectangleLines(panel_x, panel_y, panel_width, VEHICLE_PANEL_HEIGHT, vehicle_color);
    
    // Fahrzeugname und Farben
    DrawText(data.vehicle_name.c_str(), panel_x + 10, panel_y + 10, 18, vehicle_color);
    std::string color_info = data.front_color + " -> " + data.rear_color;
    DrawText(color_info.c_str(), panel_x + 10, panel_y + 35, 12, DARKGRAY);
    
    // Kompass (rechts im Panel)
    Vector2 compass_center = {panel_x + panel_width - 60, panel_y + 70};
    draw_compass(data, compass_center, 40);
    
    // Status-Indikatoren (links)
    Vector2 status_pos = {panel_x + 10, panel_y + 60};
    draw_status_indicator(data, status_pos);
    
    // Koordinaten anzeigen
    if (data.has_front && data.has_rear) {
        std::string coord_text = "Pos: (" + std::to_string((int)data.front_pos.x) + "," + 
                                std::to_string((int)data.front_pos.y) + ")";
        DrawText(coord_text.c_str(), panel_x + 10, panel_y + 110, 10, DARKGRAY);
    }
}

void MultiCarDisplay::draw_compass(const VehicleDetectionData& data, Vector2 center, float radius) {
    // Kompass-Kreis
    DrawCircleLines(center.x, center.y, radius, LIGHTGRAY);
    DrawCircleLines(center.x, center.y, radius - 5, GRAY);
    
    // Kardinalrichtungen
    DrawText("N", center.x - 5, center.y - radius - 15, 12, DARKGRAY);
    DrawText("S", center.x - 5, center.y + radius + 5, 12, DARKGRAY);
    DrawText("W", center.x - radius - 15, center.y - 5, 12, DARKGRAY);
    DrawText("E", center.x + radius + 5, center.y - 5, 12, DARKGRAY);
    
    if (data.has_angle) {
        // Richtungspfeil
        float angle_rad = data.angle_degrees * PI / 180.0f;
        Vector2 arrow_end = {
            center.x + (radius - 10) * sinf(angle_rad),
            center.y - (radius - 10) * cosf(angle_rad)
        };
        
        DrawLineEx(center, arrow_end, 3, RED);
        DrawCircle(arrow_end.x, arrow_end.y, 4, RED);
        
        // Winkel-Text
        std::string angle_text = std::to_string((int)data.angle_degrees) + "°";
        DrawText(angle_text.c_str(), center.x - 15, center.y + radius + 15, 14, DARKBLUE);
    } else {
        DrawText("---°", center.x - 15, center.y + radius + 15, 14, LIGHTGRAY);
    }
}

void MultiCarDisplay::draw_status_indicator(const VehicleDetectionData& data, Vector2 position) {
    // Vordere Farbe Status
    Color front_color = data.has_front ? GREEN : RED;
    DrawText("Vorne:", position.x, position.y, 12, DARKGRAY);
    DrawText(data.has_front ? "✓" : "✗", position.x + 50, position.y, 12, front_color);
    
    // Hintere Farbe Status
    Color rear_color = data.has_rear ? GREEN : RED;
    DrawText("Hinten:", position.x, position.y + 15, 12, DARKGRAY);
    DrawText(data.has_rear ? "✓" : "✗", position.x + 50, position.y + 15, 12, rear_color);
    
    // Richtung Status
    Color angle_color = data.has_angle ? GREEN : RED;
    DrawText("Richtung:", position.x, position.y + 30, 12, DARKGRAY);
    DrawText(data.has_angle ? "✓" : "✗", position.x + 60, position.y + 30, 12, angle_color);
}

Color MultiCarDisplay::get_vehicle_color(int index) {
    switch (index) {
        case 0: return BLUE;
        case 1: return GREEN;
        case 2: return ORANGE;
        case 3: return PURPLE;
        default: return DARKGRAY;
    }
}

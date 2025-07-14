#include "../include/CameraDisplay.h"
#include "raylib.h"
#include "../include/py_runner.h"
#include <string>
#include <chrono>
#include <ctime>

CameraDisplay::CameraDisplay() {
    // Initialize empty data
}

void CameraDisplay::update() {
    // Get new car detection data with display
    car_data = get_car_detection_with_display();
}

void CameraDisplay::draw() const {
    // Get current time for live indicator
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::tm* tm = std::localtime(&time_t);
    char time_str[50];
    sprintf(time_str, "LIVE: %02d:%02d:%02d.%03d", tm->tm_hour, tm->tm_min, tm->tm_sec, (int)ms.count());
    
    // Draw title with live indicator
    DrawText("Auto-Erkennung: Gelb=Spitze, Rot=Heck", 10, 10, 28, DARKBLUE);
    DrawText(time_str, 450, 10, 16, GREEN);
    DrawText("Kamera-Feed im separaten Fenster", 10, 45, 16, GRAY);
    
    // === AUTORICHTUNG ===
    if (car_data.has_angle) {
        // Großer Richtungsbereich
        DrawRectangleLines(10, 80, 780, 150, DARKBLUE);
        DrawText("AUTORICHTUNG", 320, 90, 24, DARKBLUE);
        
        // Richtung in Grad anzeigen
        std::string angle_text = std::to_string((int)car_data.car_angle) + "°";
        DrawText(angle_text.c_str(), 360, 120, 48, RED);
        
        // Richtungsanzeige als Pfeil
        int center_x = 200;
        int center_y = 155;
        int arrow_length = 60;
        
        // Umrechnung: 0° = nach oben (gelb oben), 90° = nach rechts
        float rad = (car_data.car_angle * PI) / 180.0f;
        int end_x = center_x + (int)(sin(rad) * arrow_length);  // sin für x-Komponente
        int end_y = center_y - (int)(cos(rad) * arrow_length);  // -cos für y-Komponente (oben = 0°)
        
        // Pfeil zeichnen
        DrawCircle(center_x, center_y, 5, DARKBLUE);
        DrawLine(center_x, center_y, end_x, end_y, DARKBLUE);
        
        // Pfeilspitze
        float arrow_angle = atan2(end_y - center_y, end_x - center_x);
        int tip1_x = end_x - (int)(cos(arrow_angle + 0.5f) * 15);
        int tip1_y = end_y - (int)(sin(arrow_angle + 0.5f) * 15);
        int tip2_x = end_x - (int)(cos(arrow_angle - 0.5f) * 15);
        int tip2_y = end_y - (int)(sin(arrow_angle - 0.5f) * 15);
        
        DrawLine(end_x, end_y, tip1_x, tip1_y, DARKBLUE);
        DrawLine(end_x, end_y, tip2_x, tip2_y, DARKBLUE);
        
        // Kompass-Beschriftung (0° = oben)
        DrawText("0°", center_x - 8, center_y - 80, 14, GRAY);
        DrawText("90°", center_x + 70, center_y - 8, 14, GRAY);
        DrawText("180°", center_x - 12, center_y + 70, 14, GRAY);
        DrawText("270°", center_x - 90, center_y - 8, 14, GRAY);
        
    } else {
        DrawRectangleLines(10, 80, 780, 150, GRAY);
        DrawText("AUTORICHTUNG", 320, 90, 24, GRAY);
        DrawText("Benötigt rote UND gelbe Objekte", 250, 140, 20, RED);
    }
    
    // === KOORDINATEN ===
    DrawText("KOORDINATEN", 10, 250, 20, DARKBLUE);
    
    // Rotes Objekt (Heck)
    int red_y_pos = 280;
    if (car_data.has_red) {
        DrawText("🔴 HECK (Rot):", 10, red_y_pos, 18, RED);
        std::string red_coords = "X: " + std::to_string(car_data.red_x) + "  Y: " + std::to_string(car_data.red_y);
        DrawText(red_coords.c_str(), 150, red_y_pos, 18, RED);
        
        // Kleine Vorschau
        DrawCircle(400, red_y_pos + 10, 8, RED);
    } else {
        DrawText("🔴 HECK (Rot):", 10, red_y_pos, 18, GRAY);
        DrawText("Nicht erkannt", 150, red_y_pos, 18, GRAY);
    }
    
    // Gelbes Objekt (Spitze)
    int yellow_y_pos = 310;
    if (car_data.has_yellow) {
        DrawText("🟡 SPITZE (Gelb):", 10, yellow_y_pos, 18, ORANGE);
        std::string yellow_coords = "X: " + std::to_string(car_data.yellow_x) + "  Y: " + std::to_string(car_data.yellow_y);
        DrawText(yellow_coords.c_str(), 170, yellow_y_pos, 18, ORANGE);
        
        // Kleine Vorschau
        DrawCircle(400, yellow_y_pos + 10, 8, YELLOW);
    } else {
        DrawText("🟡 SPITZE (Gelb):", 10, yellow_y_pos, 18, GRAY);
        DrawText("Nicht erkannt", 170, yellow_y_pos, 18, GRAY);
    }
    
    // === ZUSÄTZLICHE DATEN ===
    DrawText("ZUSÄTZLICHE DATEN", 10, 350, 20, DARKBLUE);
    
    // Entfernt: Abstandsanzeige
    DrawText("Grad-System: 0° = Gelb oben, 90° = Gelb rechts", 10, 380, 16, DARKGREEN);
    
    // Status-Übersicht
    DrawText("STATUS", 10, 420, 20, DARKBLUE);
    
    Color red_status_color = car_data.has_red ? GREEN : RED;
    Color yellow_status_color = car_data.has_yellow ? GREEN : RED;
    Color angle_status_color = car_data.has_angle ? GREEN : RED;
    
    DrawText("Rotes Objekt:", 10, 450, 14, DARKGRAY);
    DrawText(car_data.has_red ? "✓ Erkannt" : "✗ Fehlt", 120, 450, 14, red_status_color);
    
    DrawText("Gelbes Objekt:", 10, 470, 14, DARKGRAY);
    DrawText(car_data.has_yellow ? "✓ Erkannt" : "✗ Fehlt", 130, 470, 14, yellow_status_color);
    
    DrawText("Autorichtung:", 10, 490, 14, DARKGRAY);
    DrawText(car_data.has_angle ? "✓ Berechnet" : "✗ Unbekannt", 120, 490, 14, angle_status_color);
    
    // Instructions
    int blink = ((int)(GetTime() * 2)) % 2;
    Color instruction_color = blink ? DARKGRAY : GRAY;
    
    DrawText("ESC = Beenden  |  Kamera-Fenster für Live-View", 10, 550, 14, instruction_color);
    DrawText("Platziere rote und gelbe Objekte vor der Kamera", 10, 570, 14, DARKGRAY);
}

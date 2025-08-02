#include "raylib.h"
#include "../include/py_runner.h"
#include "../include/Vehicle.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

int main() {
    try {
        // Initialize Python interpreter and coordinate detector
        initialize_python();
        
        if (!initialize_coordinate_detector()) {
            std::cerr << "Fehler: Koordinaten-Detektor konnte nicht initialisiert werden!" << std::endl;
            return 1;
        }
        
        // Zuerst Raylib initialisieren
        InitWindow(800, 600, "Koordinaten-Anzeige - Initialisierung");
        
        // Dann Monitor-Größe holen
        const int MONITOR_WIDTH = GetMonitorWidth(0);
        const int MONITOR_HEIGHT = GetMonitorHeight(0);
        
        // Etwas kleineres Fenster als Monitor für Titelleiste und Taskleiste
        const int WINDOW_WIDTH = MONITOR_WIDTH - 100;
        const int WINDOW_HEIGHT = MONITOR_HEIGHT - 150;
        
        // Fenster auf gewünschte Größe setzen
        SetWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
        SetWindowTitle("Koordinaten-Anzeige - Maximiert");
        
        // Fenster in der Mitte positionieren
        SetWindowPosition(50, 50);
        
        SetTargetFPS(60);
        
        std::cout << "=== KOORDINATEN-ANZEIGE MAXIMIERT ===" << std::endl;
        std::cout << "Maximiertes Fenster " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << " mit erkannten Koordinaten" << std::endl;
        std::cout << "Monitor: " << MONITOR_WIDTH << "x" << MONITOR_HEIGHT << std::endl;
        std::cout << "ESC = Beenden | F11 = Vollbild umschalten | Fenster verschiebbar" << std::endl;
        std::cout << "====================================" << std::endl;
        
        while (!WindowShouldClose()) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                break;
            }
            
            // F11 zum Umschalten Vollbild/Fenster
            if (IsKeyPressed(KEY_F11)) {
                ToggleFullscreen();
            }
            
            // Aktuelle Fenstergröße holen
            int current_width = GetScreenWidth();
            int current_height = GetScreenHeight();
            
            // Hole aktuelle Koordinaten von der Farberkennung
            std::vector<DetectedObject> detected_objects = get_detected_coordinates();
            
            BeginDrawing();
            ClearBackground(WHITE);
            
            // Zeichne erkannte Objekte
            for (size_t i = 0; i < detected_objects.size(); i++) {
                const auto& obj = detected_objects[i];
                
                // Skaliere Koordinaten auf aktuelle Fenstergröße basierend auf echten Crop-Dimensionen
                // Koordinaten sind in Pixeln innerhalb des aktuellen Crop-Bereichs
                float scale_x = (obj.crop_width > 0) ? current_width / obj.crop_width : 1.0f;   
                float scale_y = (obj.crop_height > 0) ? current_height / obj.crop_height : 1.0f;
                float x = obj.coordinates.x * scale_x;
                float y = obj.coordinates.y * scale_y;
                
                // Stelle sicher, dass die Koordinaten im Fenster bleiben
                x = fmax(0, fmin(x, current_width - 20));
                y = fmax(0, fmin(y, current_height - 20));
                
                // Wähle Farbe basierend auf dem erkannten Objekttyp
                Color color = RED;
                if (obj.color == "Front") color = ORANGE;
                else if (obj.color == "Heck1") color = BLUE;
                else if (obj.color == "Heck2") color = GREEN;
                else if (obj.color == "Heck3") color = YELLOW;
                else if (obj.color == "Heck4") color = PURPLE;
                
                // Zeichne Punkt (größer für Vollbild)
                int point_size = current_width > 1920 ? 20 : 15; // Größere Punkte bei hoher Auflösung
                DrawCircle((int)x, (int)y, point_size, color);
                
                // Zeichne Label mit Crop-Info (größere Schrift für Vollbild)
                int font_size = current_width > 1920 ? 24 : 16;
                std::string label = obj.color + " (" + std::to_string((int)obj.coordinates.x) + "," + std::to_string((int)obj.coordinates.y) + ") " + std::to_string((int)obj.crop_width) + "x" + std::to_string((int)obj.crop_height);
                DrawText(label.c_str(), (int)x + point_size + 5, (int)y - font_size/2, font_size, BLACK);
            }
            
            // Status-Info (größere Schrift für Vollbild)
            int status_font_size = current_width > 1920 ? 32 : 20;
            std::string status = "Erkannte Objekte: " + std::to_string(detected_objects.size()) + " | Auflösung: " + std::to_string(current_width) + "x" + std::to_string(current_height);
            DrawText(status.c_str(), 20, 20, status_font_size, BLACK);
            
            // Anweisungen
            std::string instructions = "ESC=Beenden | F11=Vollbild umschalten | Fenster verschiebbar";
            DrawText(instructions.c_str(), 20, 20 + status_font_size + 10, status_font_size - 8, DARKGRAY);
            
            EndDrawing();
        }
        
        // Cleanup
        cleanup_coordinate_detector();
        CloseWindow();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

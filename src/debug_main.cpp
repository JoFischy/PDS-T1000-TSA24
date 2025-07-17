#include "raylib.h"
#include "../include/Vehicle.h"
// #include "../include/py_runner.h"  // NICHT verwenden - Konflikt mit Hauptprogramm
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <fstream>
#include <map>

int main() {
    try {
        // KEIN Python! Nur Test-Daten für Debug-Anzeige
        
        // Debug-Fenster (separates Executable ohne Python)
        InitWindow(600, 400, "DEBUG - Fahrzeugflotte Status");
        SetWindowPosition(100, 100);  // Links oben auf dem Bildschirm - SICHTBAR!
        SetTargetFPS(5);  // Langsam für Debug-Fenster
        
        std::cout << "Debug-Fenster gestartet - liest aus vehicle_data.txt!" << std::endl;
        
        while (!WindowShouldClose()) {
            // Lade echte Fahrzeugdaten aus Datei vom Hauptprogramm
            std::vector<VehicleDetectionData> vehicle_data;
            vehicle_data.resize(4);
            
            // Versuche Daten aus Datei zu laden
            std::ifstream debug_file("vehicle_data.txt");
            bool data_loaded = false;
            
            if (debug_file.is_open()) {
                std::map<std::string, std::string> data_map;
                std::string line;
                
                // Lade alle Key-Value Paare
                while (std::getline(debug_file, line)) {
                    size_t pos = line.find('=');
                    if (pos != std::string::npos) {
                        std::string key = line.substr(0, pos);
                        std::string value = line.substr(pos + 1);
                        data_map[key] = value;
                    }
                }
                debug_file.close();
                
                // Parse Fahrzeugdaten
                for (int i = 0; i < 4; i++) {
                    std::string prefix = "vehicle" + std::to_string(i) + "_";
                    
                    if (data_map.find(prefix + "detected") != data_map.end()) {
                        vehicle_data[i].detected = (data_map[prefix + "detected"] == "1");
                        vehicle_data[i].rear_color = data_map[prefix + "color"];
                        vehicle_data[i].position.x = std::stof(data_map[prefix + "x"]);
                        vehicle_data[i].position.y = std::stof(data_map[prefix + "y"]);
                        vehicle_data[i].angle = std::stof(data_map[prefix + "angle"]);
                        vehicle_data[i].distance = std::stof(data_map[prefix + "distance"]);
                        data_loaded = true;
                    }
                }
            }
            
            // Falls keine Daten geladen werden konnten - verwende Test-Daten
            if (!data_loaded) {
                // Auto-1 (Test)
                vehicle_data[0].rear_color = "Blau";
                vehicle_data[0].position = Point2D(100, 150);
                vehicle_data[0].angle = 45.5f;
                vehicle_data[0].distance = 120.0f;
                vehicle_data[0].detected = true;
                
                // Auto-2 (Test)
                vehicle_data[1].rear_color = "Grün";
                vehicle_data[1].position = Point2D(200, 250);
                vehicle_data[1].angle = 90.0f;
                vehicle_data[1].distance = 180.0f;
                vehicle_data[1].detected = true;
                
                // Auto-3 (Test)
                vehicle_data[2].rear_color = "Gelb";
                vehicle_data[2].position = Point2D(0, 0);
                vehicle_data[2].angle = 0.0f;
                vehicle_data[2].distance = 0.0f;
                vehicle_data[2].detected = false;
                
                // Auto-4 (Test)
                vehicle_data[3].rear_color = "Lila";
                vehicle_data[3].position = Point2D(300, 100);
                vehicle_data[3].angle = 135.0f;
                vehicle_data[3].distance = 220.0f;
                vehicle_data[3].detected = true;
            }
            
            BeginDrawing();
            ClearBackground(DARKBLUE);
            
            // Status-Anzeige für Datenquelle
            if (data_loaded) {
                DrawText("LIVE DATEN VOM HAUPTPROGRAMM", 50, 50, 20, GREEN);
            } else {
                DrawText("FALLBACK: TEST-DATEN", 50, 50, 20, ORANGE);
            }
            
            DrawText("Debug-Fenster aktiv", 50, 80, 16, WHITE);
            DrawText("Fenster: 600x400", 50, 110, 14, LIGHTGRAY);
            DrawText("DRÜCKE ESC ZUM BEENDEN", 50, 140, 12, RED);
            
            // Rahmen um ganzes Fenster 
            DrawRectangleLines(5, 5, 590, 390, WHITE);
            DrawRectangleLines(3, 3, 594, 394, WHITE);
            
            // Header
            DrawText("=== DEBUG FAHRZEUGFLOTTE ===", 10, 180, 16, WHITE);
            
            // Statistiken
            int detected_count = 0;
            for (const auto& vehicle : vehicle_data) {
                if (vehicle.detected) detected_count++;
            }
            
            std::ostringstream stats;
            stats << "Erkannt: " << detected_count << "/" << vehicle_data.size();
            DrawText(stats.str().c_str(), 10, 200, 14, YELLOW);
            
            // Fahrzeug-Details mit Position und Winkel
            for (size_t i = 0; i < vehicle_data.size() && i < 4; i++) {
                const auto& data = vehicle_data[i];
                int y_pos = 220 + (i * 35);
                
                // Fahrzeug-Name mit Heckfarbe
                std::ostringstream vehicle_name;
                vehicle_name << "Auto-" << (i + 1) << ": " << data.rear_color;
                if (data.detected) vehicle_name << " [OK]";
                else vehicle_name << " [---]";
                
                // Farbe je nach Status
                Color text_color = data.detected ? GREEN : RED;
                DrawText(vehicle_name.str().c_str(), 10, y_pos, 12, text_color);
                
                // Details falls erkannt
                if (data.detected) {
                    std::ostringstream details;
                    details << "Pos: (" << std::fixed << std::setprecision(0) 
                           << data.position.x << "," << data.position.y 
                           << ") Winkel: " << std::setprecision(1) << data.angle << "°";
                    DrawText(details.str().c_str(), 20, y_pos + 12, 10, LIGHTGRAY);
                    
                    std::ostringstream distance_info;
                    distance_info << "Entfernung: " << std::setprecision(0) << data.distance << "cm";
                    DrawText(distance_info.str().c_str(), 20, y_pos + 22, 10, LIGHTGRAY);
                }
            }
            
            // Zeitstempel
            std::string data_source = data_loaded ? "Live-Daten aktiv" : "Test-Daten (kein Hauptprogramm)";
            DrawText(data_source.c_str(), 10, 370, 10, GRAY);
            
            EndDrawing();
        }
        
        // Cleanup - KEIN cleanup_vehicle_fleet() da es Python benötigt!
        CloseWindow();
        
        std::cout << "Debug-Fenster beendet" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Debug-Fenster Fehler: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

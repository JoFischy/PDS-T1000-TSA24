
#include "raylib.h"
#include "../include/MultiCarDisplay.h"
#include "../include/py_runner.h"
#include <iostream>

int main() {
    try {
        // Initialize Python interpreter and vehicle fleet
        initialize_python();
        
        if (!initialize_vehicle_fleet()) {
            std::cerr << "Fehler: Fahrzeugflotte konnte nicht initialisiert werden!" << std::endl;
            return 1;
        }
        
        InitWindow(1000, 700, "FAHRZEUGFLOTTE - 4 Autos mit Farberkennung");
        MultiCarDisplay fleet_display;
        SetTargetFPS(30);
        
        std::cout << "=== FAHRZEUGFLOTTE GESTARTET ===" << std::endl;
        std::cout << "EINHEITLICHE VORDERE FARBE: GELB" << std::endl;
        std::cout << "Auto-1: Gelb vorne, Rot hinten" << std::endl;
        std::cout << "Auto-2: Gelb vorne, Blau hinten" << std::endl;
        std::cout << "Auto-3: Gelb vorne, Grün hinten" << std::endl;
        std::cout << "Auto-4: Gelb vorne, Lila hinten" << std::endl;
        std::cout << "Intelligente Paar-Zuordnung aktiv!" << std::endl;
        std::cout << "=================================" << std::endl;
        
        while (!WindowShouldClose()) {
            // Update fleet detection and display camera feed
            fleet_display.update();
            show_fleet_camera_feed();
            
            // Handle OpenCV window events
            handle_opencv_events();
            
            BeginDrawing();
            ClearBackground(RAYWHITE);
            fleet_display.draw();
            EndDrawing();
        }
        
        // Cleanup
        cleanup_vehicle_fleet();
        CloseWindow();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

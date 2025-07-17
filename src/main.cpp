#include "raylib.h"
#include "../include/MultiCarDisplay.h"
#include "../include/BeamerProjection.h"
#include "../include/py_runner.h"
#include <iostream>
#include <chrono>

int main() {
    try {
        // Initialize Python interpreter and vehicle fleet
        initialize_python();
        
        if (!initialize_vehicle_fleet()) {
            std::cerr << "Fehler: Fahrzeugflotte konnte nicht initialisiert werden!" << std::endl;
            return 1;
        }
        
        // Quadratisches Hauptfenster in der Bildschirmmitte (800x800)
        const int WINDOW_SIZE = 800;
        InitWindow(WINDOW_SIZE, WINDOW_SIZE, "BEAMER PROJEKTION - Fahrzeug-Positionen");
        
        // Zentriere das Fenster auf dem Bildschirm
        int monitor_width = GetMonitorWidth(0);
        int monitor_height = GetMonitorHeight(0);
        int window_x = (monitor_width - WINDOW_SIZE) / 2;
        int window_y = (monitor_height - WINDOW_SIZE) / 2;
        SetWindowPosition(window_x, window_y);
        
        BeamerProjection beamer;
        beamer.initialize();
        SetTargetFPS(30);
        
        std::cout << "=== FAHRZEUGFLOTTE GESTARTET ===" << std::endl;
        std::cout << "EINHEITLICHE VORDERE FARBE: ORANGE" << std::endl;
        std::cout << "Auto-1: Orange vorne, Blau hinten" << std::endl;
        std::cout << "Auto-2: Orange vorne, Grün hinten" << std::endl;
        std::cout << "Auto-3: Orange vorne, Gelb hinten" << std::endl;
        std::cout << "Auto-4: Orange vorne, Lila hinten" << std::endl;
        std::cout << "Intelligente Paar-Zuordnung aktiv!" << std::endl;
        std::cout << "=================================" << std::endl;
        
        while (!WindowShouldClose()) {
            // Hole aktuelle Fahrzeugdaten
            std::vector<VehicleDetectionData> vehicle_data = get_all_vehicle_detections();
            
            // Update Beamer-Projektion
            beamer.update(vehicle_data);
            show_fleet_camera_feed();  // Zeigt OpenCV Debug-Fenster
            
            // Konsolen-Debug-Ausgabe
            static int debug_counter = 0;
            if (++debug_counter % 30 == 0) {  // Alle 30 Frames (1 Sekunde)
                int detected = 0;
                for (const auto& v : vehicle_data) if (v.detected) detected++;
                std::cout << "STATUS: " << detected << "/" << vehicle_data.size() << " Fahrzeuge erkannt" << std::endl;
            }
            
            // Handle OpenCV window events
            handle_opencv_events();
            
            // Render nur Beamer-Projektion
            beamer.draw();
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

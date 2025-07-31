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
        
        // Großes verschiebbares Fenster für Beamer-Projektion
        // Initialisiere erst mit einer Standardgröße
        const int DEFAULT_WIDTH = 1600;
        const int DEFAULT_HEIGHT = 1200;
        
        InitWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "BEAMER PROJEKTION - Boden-Markierung");
        
        // Jetzt können wir die Monitor-Größe abfragen
        int monitor_width = GetMonitorWidth(0);
        int monitor_height = GetMonitorHeight(0);
        
        // Fenster auf optimale Größe anpassen (etwas kleiner als Monitor)
        int window_width = monitor_width - 100;
        int window_height = monitor_height - 100;
        
        // Stelle sicher, dass die Werte positiv sind
        if (window_width < 800) window_width = 800;
        if (window_height < 600) window_height = 600;
        
        // Fenstergröße anpassen
        SetWindowSize(window_width, window_height);
        
        // Fenster zentrieren
        SetWindowPosition((monitor_width - window_width) / 2, (monitor_height - window_height) / 2);
        
        // Fenster kann vergrößert werden
        SetWindowState(FLAG_WINDOW_RESIZABLE);
        
        BeamerProjection beamer;
        beamer.initialize();
        SetTargetFPS(30);
        
        std::cout << "=== BEAMER-PROJEKTION FENSTER ===" << std::endl;
        std::cout << "4 ROTE ECKPUNKTE = Kamera-Erkennungsbereich (maximaler Kontrast!)" << std::endl;
        std::cout << "ESC = Beenden | F11 = Vollbild umschalten" << std::endl;
        std::cout << "Fenster ist verschiebbar und vergrößerbar" << std::endl;
        std::cout << "SCHWARZE FAHRZEUGE mit MARKIERUNGSPUNKTEN:" << std::endl;
        std::cout << "Auto-1: Orange vorne, Helles Blau hinten" << std::endl;
        std::cout << "Auto-2: Orange vorne, Reines Grün hinten" << std::endl;
        std::cout << "Auto-3: Orange vorne, Reines Gelb hinten" << std::endl;
        std::cout << "Auto-4: Orange vorne, Reines Magenta hinten" << std::endl;
        std::cout << "Optimiert für weiße Projektionsfläche!" << std::endl;
        std::cout << "==================================" << std::endl;
        
        while (!WindowShouldClose()) {
            // ESC-Taste zum Verlassen des Vollbildmodus
            if (IsKeyPressed(KEY_ESCAPE)) {
                break;
            }
            
            // F11 zum Umschalten Vollbild/Fenster
            if (IsKeyPressed(KEY_F11)) {
                ToggleFullscreen();
            }
            
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

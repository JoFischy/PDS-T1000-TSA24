
#include <iostream>
#include <vector>
#include "car_simulation.h"
#include "py_runner.h"
#include "test_window.h"

int main(int argc, char* argv[]) {
    // Command-line Parameter parsen
    bool auto_fullscreen = false;
    bool auto_fullscreen_monitor2 = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--fullscreen" || arg == "-f") {
            auto_fullscreen = true;
        } else if (arg == "--monitor2" || arg == "-m2") {
            auto_fullscreen_monitor2 = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Verwendung: " << argv[0] << " [OPTIONEN]" << std::endl;
            std::cout << "  --fullscreen, -f     Vollbild auf aktuellem Monitor" << std::endl;
            std::cout << "  --monitor2, -m2      Vollbild auf Monitor 2" << std::endl;
            std::cout << "  --help, -h           Diese Hilfe anzeigen" << std::endl;
            return 0;
        }
    }
    
    // Standard-Fenstergröße
    int screenWidth = 1200;
    int screenHeight = 800;
    
    // Raylib Monitor-Detection - erst Fenster initialisieren für Monitor-Abfrage
    SetTraceLogLevel(LOG_WARNING); // Reduziere Logs
    
    if (auto_fullscreen_monitor2) {
        // Monitor 2 Setup - minimal output
        InitWindow(800, 600, "PDS-T1000-TSA24");
        
        int monitorCount = GetMonitorCount();
        
        if (monitorCount >= 2) {
            Vector2 monitor2Pos = GetMonitorPosition(1);
            screenWidth = GetMonitorWidth(1);
            screenHeight = GetMonitorHeight(1);
            
            WaitTime(0.1f);
            SetWindowPosition((int)monitor2Pos.x + 100, (int)monitor2Pos.y + 100);
            WaitTime(0.1f);
            SetWindowSize(screenWidth - 200, screenHeight - 200);
            WaitTime(0.1f);
            ToggleFullscreen();
        } else {
            ToggleFullscreen();
        }
    } else if (auto_fullscreen) {
        InitWindow(screenWidth, screenHeight, "PDS-T1000-TSA24");
        ToggleFullscreen();
    } else {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT); // VSync für weniger Flackern
        InitWindow(screenWidth, screenHeight, "PDS-T1000-TSA24");
        MaximizeWindow();
    }
    
    SetTargetFPS(60);
    
    // === SEPARATES TEST-FENSTER STARTEN ===
    // Startet ein maximiertes Live-Koordinaten-Fenster auf dem Hauptmonitor
    #ifdef _WIN32
    createWindowsAPITestWindow();
    std::cout << "Live-Koordinaten-Fenster wird maximiert auf Hauptmonitor gestartet..." << std::endl;
    #endif
    
    // === CV2-FENSTER SETUP FÜR PYTHON ===
    // Positioniere CV2-Fenster auf einem anderen Monitor als Raylib
    int monitorCount = GetMonitorCount();
    
    if (auto_fullscreen_monitor2) {
        // Raylib läuft auf Monitor 2, CV2-Fenster sollen auf Monitor 3 (wenn verfügbar)
        std::cout << "Raylib läuft auf Monitor 2 - CV2-Fenster werden auf Monitor 3 positioniert" << std::endl;
        
        if (monitorCount >= 3) {
            // Verwende Monitor 3 für CV2-Fenster
            Vector2 monitor3Pos = GetMonitorPosition(2);  // Monitor 3 (Index 2)
            std::cout << "Monitor 3 verfügbar bei Position: " << monitor3Pos.x << ", " << monitor3Pos.y << std::endl;
            
            if (set_python_monitor3_position((int)monitor3Pos.x + 50, (int)monitor3Pos.y + 50)) {
                std::cout << "CV2-Fenster erfolgreich auf Monitor 3 konfiguriert" << std::endl;
            } else {
                std::cout << "Warnung: Monitor 3 Konfiguration fehlgeschlagen" << std::endl;
            }
        } else {
            // Fallback: Verwende Monitor 1 für CV2-Fenster wenn kein Monitor 3
            Vector2 monitor1Pos = GetMonitorPosition(0);  // Monitor 1 (Index 0)
            std::cout << "Nur 2 Monitore - CV2-Fenster werden auf Monitor 1 (Primär) positioniert" << std::endl;
            
            if (set_python_monitor3_position((int)monitor1Pos.x + 50, (int)monitor1Pos.y + 50)) {
                std::cout << "CV2-Fenster erfolgreich auf Monitor 1 konfiguriert" << std::endl;
            }
        }
    } else {
        // Standard-Modus: Verwende Monitor 3 wenn verfügbar, sonst Monitor 2
        if (monitorCount >= 3) {
            Vector2 monitor3Pos = GetMonitorPosition(2);
            if (set_python_monitor3_position((int)monitor3Pos.x + 50, (int)monitor3Pos.y + 50)) {
                std::cout << "CV2-Fenster auf Monitor 3 positioniert" << std::endl;
            }
        } else if (monitorCount >= 2) {
            Vector2 monitor2Pos = GetMonitorPosition(1);
            if (set_python_monitor3_position((int)monitor2Pos.x, (int)monitor2Pos.y)) {
                std::cout << "CV2-Fenster auf Monitor 2 positioniert" << std::endl;
            }
        }
    }
    
    // Aktualisiere die Fensterdimensionen
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();
    
    // Create car simulation with new point system
    CarSimulation car_simulation;
    car_simulation.initialize();
    car_simulation.setCarPointDistance(12.0f);  // Set distance between front and ID points
    car_simulation.setDistanceBuffer(4.0f);     // Set tolerance buffer for pairing
    
    // Vollbild-Transformation: Gesamte Fensterfläche = Crop-Bereich
    FieldTransform field_transform;
    field_transform.field_cols = FIELD_WIDTH;
    field_transform.field_rows = FIELD_HEIGHT;
    field_transform.field_width = currentWidth;   // Gesamte Fensterbreite
    field_transform.field_height = currentHeight; // Gesamte Fensterhöhe
    field_transform.offset_x = 0;                 // Kein Offset - gesamte Fläche nutzen
    field_transform.offset_y = 0;                 // Kein Offset - gesamte Fläche nutzen
    
    // Monitor-Konfiguration für später speichern
    static bool monitor_config_pending = false;
    static int pending_monitor_x = 0;
    static int pending_monitor_y = 0;
    
    // Speichere Monitor-Position für nachträgliche Konfiguration
    if (auto_fullscreen_monitor2 && monitorCount >= 3) {
        Vector2 monitor3Pos = GetMonitorPosition(2);
        monitor_config_pending = true;
        pending_monitor_x = (int)monitor3Pos.x + 50;
        pending_monitor_y = (int)monitor3Pos.y + 50;
    }
    
    // EINFACHE HAUPTSCHLEIFE
    while (!WindowShouldClose()) {
        // Nur ESC zum Beenden
        if (IsKeyPressed(KEY_ESCAPE)) {
            break;
        }
        
        float deltaTime = GetFrameTime();
        
        // Hole aktuelle Koordinaten von der Farberkennung
        std::vector<DetectedObject> detected_objects = get_detected_coordinates();
        
        // Update Live-Koordinaten-Fenster
        #ifdef _WIN32
        updateTestWindowCoordinates(detected_objects);
        #endif
        
        // Konfiguriere Monitor-Position nachträglich, wenn nötig
        if (monitor_config_pending) {
            configure_monitor_position_delayed(pending_monitor_x, pending_monitor_y);
            monitor_config_pending = false; // Nur einmal versuchen
        }
        
        // Update car simulation with real detected objects (mit Vollbild-Koordinaten)
        car_simulation.updateFromDetectedObjects(detected_objects, field_transform);
        car_simulation.update(deltaTime);
        
        BeginDrawing();
        ClearBackground(WHITE); // Weißer Hintergrund für gesamte Fläche
        
        // Render all UI, points, and cars through the integrated renderer
        car_simulation.renderUI();
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}

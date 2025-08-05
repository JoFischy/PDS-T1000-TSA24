#include <iostream>
#include <vector>
#include "car_simulation.h"
#include "py_runner.h"

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
    int target_monitor = 0; // Standard: Monitor 1
    
    // Raylib Monitor-Detection OHNE Fenster zu öffnen
    SetTraceLogLevel(LOG_WARNING); // Reduziere Logs
    
    if (auto_fullscreen_monitor2) {
        target_monitor = 1; // Monitor 2 - vertraue darauf dass er existiert
        screenWidth = 1920;  // Feste Werte für Monitor 2
        screenHeight = 1200;
        std::cout << "ZIEL: Vollbild auf Monitor 2 (1920x1200)" << std::endl;
    }
    
    // Jetzt das Fenster mit korrekten Einstellungen erstellen
    if (auto_fullscreen || auto_fullscreen_monitor2) {
        SetConfigFlags(FLAG_WINDOW_UNDECORATED);
        
        if (auto_fullscreen_monitor2) {
            // Spezielle Position für Monitor 2
            InitWindow(screenWidth, screenHeight, "PDS-T1000-TSA24");
            SetWindowPosition(1920, 0); // Harte Position für Monitor 2
        } else {
            InitWindow(screenWidth, screenHeight, "PDS-T1000-TSA24");
        }
        std::cout << "✅ VOLLBILD AUTOMATISCH AKTIVIERT!" << std::endl;
    } else {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(screenWidth, screenHeight, "PDS-T1000-TSA24");
        MaximizeWindow();
    }
    
    SetTargetFPS(60);
    
    // Aktualisiere die Fensterdimensionen
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();
    
    // Create car simulation
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
    
    std::cout << "=== PDS-T1000-TSA24 AUTOMATISCHES VOLLBILD ===" << std::endl;
    std::cout << "Gesamte Fensterfläche repräsentiert den Crop-Bereich" << std::endl;
    std::cout << "Fenster: " << currentWidth << "x" << currentHeight << " Pixel" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "🎯 VERWENDUNG:" << std::endl;
    std::cout << "NORMALE NUTZUNG: .\\main.exe" << std::endl;
    std::cout << "VOLLBILD MONITOR 2: .\\main.exe --monitor2" << std::endl;
    std::cout << "VOLLBILD AKTUELLER MONITOR: .\\main.exe --fullscreen" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "ESC oder Fenster schließen = Beenden" << std::endl;
    std::cout << "KEINE TASTEN NÖTIG - alles automatisch!" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    // EINFACHE HAUPTSCHLEIFE OHNE TASTEN-HANDLING
    while (!WindowShouldClose()) {
        // Nur ESC zum Beenden (falls es funktioniert)
        if (IsKeyPressed(KEY_ESCAPE)) {
            break;
        }
        
        float deltaTime = GetFrameTime();
        
        // Hole aktuelle Koordinaten von der Farberkennung
        std::vector<DetectedObject> detected_objects = get_detected_coordinates();
        
        // Update car simulation with real detected objects (mit Vollbild-Koordinaten)
        car_simulation.updateFromDetectedObjects(detected_objects, field_transform);
        car_simulation.update(deltaTime);
        
        BeginDrawing();
        ClearBackground(WHITE); // Weißer Hintergrund für gesamte Fläche
        
        // Render Punkte und Autos auf weißem Hintergrund
        car_simulation.renderPoints();
        car_simulation.renderCars();
        
        // Render minimale UI mit Koordinaten-Anzeige
        car_simulation.renderUI();
        
        // Debug-Info falls keine Objekte erkannt werden (zentriert)
        if (detected_objects.empty()) {
            int currentWidth = GetScreenWidth();
            int currentHeight = GetScreenHeight();
            const char* text = "WARTEN AUF KAMERA-DATEN...";
            int text_width = MeasureText(text, 24);
            DrawText(text, (currentWidth - text_width) / 2, currentHeight / 2, 24, RED);
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}

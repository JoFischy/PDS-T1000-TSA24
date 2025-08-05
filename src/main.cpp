#include "raylib.h"
#include "../include/py_runner.h"
#include "../include/Vehicle.h"
#include "../include/car_simulation.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

// Forward-Deklarationen für Koordinatenumrechnung
void cameraToWindow(const DetectedObject& obj, const FieldTransform& transform, float& window_x, float& window_y);
void cameraToField(const DetectedObject& obj, const FieldTransform& transform, int& field_col, int& field_row);

// Konvertiert Kamera-Koordinaten direkt zu Vollbild-Fenster-Pixeln (gesamte Fläche = Crop)
void cameraToWindow(const DetectedObject& obj, const FieldTransform& transform, float& window_x, float& window_y) {
    if (obj.crop_width <= 0 || obj.crop_height <= 0) {
        std::cout << "DEBUG: Ungültige Crop-Dimensionen: " << obj.crop_width << "x" << obj.crop_height << std::endl;
        window_x = window_y = 0;
        return;
    }
    
    // Debug-Ausgabe der eingehenden Daten
    std::cout << "DEBUG: Objekt " << obj.color << " - Coords: (" << obj.coordinates.x << ", " << obj.coordinates.y 
              << ") in Crop: " << obj.crop_width << "x" << obj.crop_height << std::endl;
    
    // Normalisiere Kamera-Koordinaten (0.0 bis 1.0)
    float norm_x = obj.coordinates.x / obj.crop_width;
    float norm_y = obj.coordinates.y / obj.crop_height;
    
    std::cout << "DEBUG: Normalisiert: (" << norm_x << ", " << norm_y << ")" << std::endl;
    
    // Mappe direkt auf die gesamte Fensterfläche (gesamte Fläche = Crop-Bereich)
    window_x = norm_x * transform.field_width;
    window_y = norm_y * transform.field_height;
    
    std::cout << "DEBUG: Vollbild-Position: (" << window_x << ", " << window_y << ") im " 
              << transform.field_width << "x" << transform.field_height << " Fenster" << std::endl;
}

// Legacy-Funktion für Spielfeld-Koordinaten (falls noch benötigt)
void cameraToField(const DetectedObject& obj, const FieldTransform& transform, int& field_col, int& field_row) {
    if (obj.crop_width <= 0 || obj.crop_height <= 0) {
        field_col = field_row = 0;
        return;
    }
    
    // Normalisiere Kamera-Koordinaten (0.0 bis 1.0)
    float norm_x = obj.coordinates.x / obj.crop_width;
    float norm_y = obj.coordinates.y / obj.crop_height;
    
    // Mappe auf Spielfeld-Spalten/Zeilen (ganze Zahlen)
    field_col = (int)(norm_x * transform.field_cols);
    field_row = (int)(norm_y * transform.field_rows);
    
    // Begrenze auf Spielfeld
    field_col = fmax(0, fmin(field_col, transform.field_cols - 1));
    field_row = fmax(0, fmin(field_row, transform.field_rows - 1));
}

// Konvertiert Spielfeld-Koordinaten (Spalte/Zeile) zu Fenster-Pixeln (Feldmitte)
void fieldToWindow(int field_col, int field_row, const FieldTransform& transform, float& window_x, float& window_y) {
    // Zentriere in der Mitte des Feldes
    window_x = transform.offset_x + (field_col + 0.5f) * FIELD_SIZE;
    window_y = transform.offset_y + (field_row + 0.5f) * FIELD_SIZE;
}

int main() {
    try {
        // Initialize Python interpreter and coordinate detector
        initialize_python();
        
        if (!initialize_coordinate_detector()) {
            std::cerr << "Fehler: Koordinaten-Detektor konnte nicht initialisiert werden!" << std::endl;
            return 1;
        }
        
        // Initialize Raylib window maximiert (aber nicht Vollbild, damit verschiebbar)
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        
        // Hole die Bildschirmauflösung
        int monitor = GetCurrentMonitor();
        int screenWidth = GetMonitorWidth(monitor);
        int screenHeight = GetMonitorHeight(monitor);
        
        InitWindow(screenWidth, screenHeight, "PDS-T1000-TSA24 - Crop-Bereich (verschiebbar)");
        
        // Maximiere das Fenster nach der Erstellung (damit es verschiebbar bleibt)
        MaximizeWindow();
        SetTargetFPS(60);
        
        // Aktualisiere die Fensterdimensionen
        int currentWidth = GetScreenWidth();
        int currentHeight = GetScreenHeight();
        
        std::cout << "Fenstergröße: " << currentWidth << "x" << currentHeight << " (verschiebbar auf zweiten Monitor)" << std::endl;
        
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
        
        std::cout << "=== PDS-T1000-TSA24 CROP-BEREICH MAXIMIERT ===" << std::endl;
        std::cout << "Gesamte Fensterfläche repräsentiert den Crop-Bereich" << std::endl;
        std::cout << "Fenster: " << currentWidth << "x" << currentHeight << " Pixel (verschiebbar)" << std::endl;
        std::cout << "ESC = Beenden | Fenster auf zweiten Monitor verschiebbar" << std::endl;
        std::cout << "Kamera-Fenster für Crop-Anpassung werden geöffnet" << std::endl;
        std::cout << "=================================================" << std::endl;
        
        while (!WindowShouldClose()) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                break;
            }
            
            if (IsKeyPressed(KEY_F11)) {
                ToggleFullscreen();
            }
            
            float deltaTime = GetFrameTime();
            
            // Hole aktuelle Koordinaten von der Farberkennung
            std::vector<DetectedObject> detected_objects = get_detected_coordinates();
            
            // Update car simulation with real detected objects (mit Vollbild-Koordinaten)
            car_simulation.updateFromDetectedObjects(detected_objects, field_transform);
            car_simulation.update(deltaTime);
            
            BeginDrawing();
            ClearBackground(WHITE); // Weißer Hintergrund für gesamte Fläche
            
            // Render Punkte direkt ohne Spielfeld-Rahmen
            car_simulation.renderPoints();
            car_simulation.renderCars();
            
            // Minimale Info-Anzeige in der Ecke
            DrawText(TextFormat("Crop-Bereich: %dx%d", currentWidth, currentHeight), 10, 10, 20, BLACK);
            DrawText("ESC=Beenden | Fenster verschiebbar", 10, currentHeight - 30, 16, DARKGRAY);
            
            // Debug-Info falls keine Objekte erkannt werden
            if (detected_objects.empty()) {
                DrawText("WARTEN AUF KAMERA-DATEN...", currentWidth/2 - 150, currentHeight/2, 24, RED);
            }
            
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

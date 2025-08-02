#include "raylib.h"
#include "../include/py_runner.h"
#include "../include/Vehicle.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

// Definiertes Spielfeld-Koordinatensystem mit quadratischen Feldern
const int FIELD_SIZE = 10;       // Größe eines quadratischen Feldes in Pixeln (4x kleiner)
const int UI_HEIGHT = 80;        // Höhe für Status-UI oben

// Spielfeld-zu-Fenster Transformation
struct FieldTransform {
    int field_cols, field_rows;     // Anzahl Spalten und Zeilen
    int field_width, field_height;  // Spielfeld-Dimensionen in virtuellen Einheiten
    float offset_x, offset_y;       // Zentrierung
    
    void calculate(int window_width, int window_height) {
        // Verfügbare Fläche für das Spielfeld
        int available_width = window_width;
        int available_height = window_height - UI_HEIGHT;
        
        // Berechne Anzahl quadratischer Felder die reinpassen
        field_cols = available_width / FIELD_SIZE;
        field_rows = available_height / FIELD_SIZE;
        
        // Tatsächliche Spielfeld-Dimensionen
        field_width = field_cols * FIELD_SIZE;
        field_height = field_rows * FIELD_SIZE;
        
        // Zentrierung berechnen
        offset_x = (window_width - field_width) / 2.0f;
        offset_y = UI_HEIGHT + (available_height - field_height) / 2.0f;
    }
};

// Konvertiert Kamera-Koordinaten zu Spielfeld-Koordinaten (Ganzzahl-Snap)
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
        
        std::cout << "=== KOORDINATEN-ANZEIGE MIT QUADRATISCHEM SPIELFELD ===" << std::endl;
        std::cout << "Maximiertes Fenster " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << " mit erkannten Koordinaten" << std::endl;
        std::cout << "Monitor: " << MONITOR_WIDTH << "x" << MONITOR_HEIGHT << std::endl;
        std::cout << "Quadratische Felder: " << FIELD_SIZE << "x" << FIELD_SIZE << " Pixel pro Feld" << std::endl;
        std::cout << "ESC = Beenden | F11 = Vollbild umschalten | Fenster verschiebbar" << std::endl;
        std::cout << "=======================================================" << std::endl;
        
        // Spielfeld-Transformation
        FieldTransform field_transform;
        
        while (!WindowShouldClose()) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                break;
            }
            
            // F11 zum Umschalten Vollbild/Fenster
            if (IsKeyPressed(KEY_F11)) {
                ToggleFullscreen();
            }
            
            // Aktuelle Fenstergröße holen und Spielfeld-Transformation berechnen
            int current_width = GetScreenWidth();
            int current_height = GetScreenHeight();
            field_transform.calculate(current_width, current_height);
            
            // Hole aktuelle Koordinaten von der Farberkennung
            std::vector<DetectedObject> detected_objects = get_detected_coordinates();
            
            BeginDrawing();
            ClearBackground(WHITE);
            
            // Zeichne Spielfeld-Raster
            Color grid_color = LIGHTGRAY;
            for (int x = 0; x <= field_transform.field_cols; x++) {
                int line_x = (int)(field_transform.offset_x + x * FIELD_SIZE);
                DrawLine(line_x, (int)field_transform.offset_y, 
                        line_x, (int)(field_transform.offset_y + field_transform.field_height), grid_color);
            }
            for (int y = 0; y <= field_transform.field_rows; y++) {
                int line_y = (int)(field_transform.offset_y + y * FIELD_SIZE);
                DrawLine((int)field_transform.offset_x, line_y, 
                        (int)(field_transform.offset_x + field_transform.field_width), line_y, grid_color);
            }
            
            // Zeichne Spielfeld-Rahmen
            DrawRectangleLines((int)field_transform.offset_x, (int)field_transform.offset_y, 
                             field_transform.field_width, field_transform.field_height, BLACK);
            
            // Zeichne erkannte Objekte
            for (size_t i = 0; i < detected_objects.size(); i++) {
                const auto& obj = detected_objects[i];
                
                // Konvertiere zu Spielfeld-Koordinaten (ganze Zahlen) und dann zu Fenster-Pixeln
                int field_col, field_row;
                cameraToField(obj, field_transform, field_col, field_row);
                
                float window_x, window_y;
                fieldToWindow(field_col, field_row, field_transform, window_x, window_y);
                
                // Wähle Farbe basierend auf dem erkannten Objekttyp
                Color color = RED;
                if (obj.color == "Front") color = ORANGE;
                else if (obj.color == "Heck1") color = BLUE;
                else if (obj.color == "Heck2") color = GREEN;
                else if (obj.color == "Heck3") color = YELLOW;
                else if (obj.color == "Heck4") color = PURPLE;
                
                // Zeichne Punkt (kleiner wegen kleineren Feldern)
                int point_size = FIELD_SIZE / 2; // 5px bei 10px Feldern
                DrawCircle((int)window_x, (int)window_y, point_size, color);
                
                // Zeichne Label mit Spielfeld-Koordinaten
                int font_size = 8; // Kleinere Schrift für kleinere Felder
                std::string label = obj.color + " [" + std::to_string(field_col) + "," + std::to_string(field_row) + "] C(" + std::to_string((int)obj.coordinates.x) + "," + std::to_string((int)obj.coordinates.y) + ")";
                DrawText(label.c_str(), (int)window_x + point_size + 2, (int)window_y - font_size/2, font_size, BLACK);
            }
            
            // Status-Info
            int status_font_size = 20;
            std::string status = "Objekte: " + std::to_string(detected_objects.size()) + " | Spielfeld: " + std::to_string(field_transform.field_cols) + "x" + std::to_string(field_transform.field_rows) + " Felder | Auflösung: " + std::to_string(current_width) + "x" + std::to_string(current_height);
            DrawText(status.c_str(), 20, 20, status_font_size, BLACK);
            
            // Anweisungen
            std::string instructions = "ESC=Beenden | F11=Vollbild umschalten | Punkt-Snap auf Feldmitte";
            DrawText(instructions.c_str(), 20, 45, 16, DARKGRAY);
            
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

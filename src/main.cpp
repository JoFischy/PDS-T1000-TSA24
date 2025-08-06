#include "raylib.h"
#include "../include/py_runner.h"
#include "../include/Vehicle.h"
#include "../include/car_simulation.h"
#include "../include/uart_communication.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <chrono>

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
        
        // Initialize UART communication for ESP32
        UARTCommunication uart("COM5");
        bool uart_connected = uart.initialize();
        if (!uart_connected) {
            std::cout << "Warnung: UART Verbindung zu ESP32 auf COM5 fehlgeschlagen" << std::endl;
            std::cout << "Das Programm läuft weiter, aber Koordinaten werden nicht an ESP32 gesendet" << std::endl;
        }
        
        // Initialize Raylib window
        InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "PDS-T1000-TSA24 Car Detection - 182x93 Field + UART");
        SetTargetFPS(60);
        
        // Create car simulation
        CarSimulation car_simulation;
        car_simulation.initialize();
        car_simulation.setCarPointDistance(12.0f);  // Set distance between front and ID points
        car_simulation.setDistanceBuffer(4.0f);     // Set tolerance buffer for pairing
        
        // Spielfeld-Transformation für Koordinaten-Umrechnung
        FieldTransform field_transform;
        field_transform.field_cols = FIELD_WIDTH;
        field_transform.field_rows = FIELD_HEIGHT;
        field_transform.field_width = FIELD_WIDTH * FIELD_SIZE;
        field_transform.field_height = FIELD_HEIGHT * FIELD_SIZE;
        field_transform.offset_x = (WINDOW_WIDTH - field_transform.field_width) / 2.0f;
        field_transform.offset_y = UI_HEIGHT + (WINDOW_HEIGHT - UI_HEIGHT - field_transform.field_height) / 2.0f;
        
        std::cout << "=== PDS-T1000-TSA24 CAR DETECTION SYSTEM ===" << std::endl;
        std::cout << "Field: " << FIELD_WIDTH << "x" << FIELD_HEIGHT << " mit echten Koordinaten" << std::endl;
        std::cout << "UART: " << (uart_connected ? "ESP32 verbunden auf COM5" : "ESP32 nicht verbunden") << std::endl;
        std::cout << "ESC = Beenden | F11 = Vollbild umschalten" << std::endl;
        std::cout << "Kamera-Fenster für Crop-Anpassung werden geöffnet" << std::endl;
        std::cout << "UART sendet ALLE Koordinaten live an ESP32 für ESP-NOW" << std::endl;
        std::cout << "=============================================" << std::endl;
        
        // Timer für UART-Übertragung (alle 100ms für Live-Updates)
        auto last_uart_send = std::chrono::steady_clock::now();
        
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
            
            // UART-Übertragung ALLER erkannten Koordinaten (alle 100ms für Live-Update)
            auto now = std::chrono::steady_clock::now();
            if (uart_connected && !detected_objects.empty() && 
                (now - last_uart_send) >= std::chrono::milliseconds(100)) {
                
                // Sende alle erkannten Objekte an ESP32
                for (const auto& obj : detected_objects) {
                    std::string message = "COORD:" + obj.color + ":" + 
                                        std::to_string(obj.coordinates.x) + "," + 
                                        std::to_string(obj.coordinates.y) + "\n";
                    
                    bool sent = uart.sendMessage(message);
                    if (sent) {
                        std::cout << "Gesendet: " << obj.color << " (" << 
                                     obj.coordinates.x << "," << obj.coordinates.y << ")" << std::endl;
                    }
                }
                last_uart_send = now;
            }
            
            // Update car simulation with real detected objects
            car_simulation.updateFromDetectedObjects(detected_objects, field_transform);
            car_simulation.update(deltaTime);
            
            BeginDrawing();
            ClearBackground(DARKGRAY);
            
            // Render car simulation
            car_simulation.renderField();
            car_simulation.renderPoints();
            car_simulation.renderCars();
            car_simulation.renderUI();
            
            // Additional info overlay
            DrawText("PDS-T1000-TSA24 - Real-time Car Detection + UART ESP32", 10, WINDOW_HEIGHT - 40, 16, WHITE);
            std::string uart_status = uart_connected ? "UART: COM5 Connected - Sending ALL coordinates LIVE" : "UART: COM5 Disconnected";
            DrawText(uart_status.c_str(), 10, WINDOW_HEIGHT - 20, 12, uart_connected ? GREEN : RED);
            
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

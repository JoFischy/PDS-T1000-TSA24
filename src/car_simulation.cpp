#include "../include/car_simulation.h"
#include "../include/Vehicle.h"
#include <algorithm>
#include <limits>
#include <cstdio>

#ifndef M_PI
#define M_PI PI
#endif

// Implementation of FieldTransform::calculate method
void FieldTransform::calculate(int window_width, int window_height) {
    // GESAMTE Fensterfläche nutzen - KEIN UI_HEIGHT Abzug mehr!
    int available_width = window_width;
    int available_height = window_height;  // Komplett ohne UI-Abzug
    
    // Berechne Anzahl quadratischer Felder die reinpassen
    field_cols = available_width / FIELD_SIZE;
    field_rows = available_height / FIELD_SIZE;
    
    // Tatsächliche Spielfeld-Dimensionen = GESAMTE Fensterfläche
    field_width = window_width;   // Komplette Breite
    field_height = window_height; // Komplette Höhe
    
    // Kein Offset - nutze die gesamte Fläche
    offset_x = 0;
    offset_y = 0;
}

CarSimulation::CarSimulation() : time_elapsed(0.0f), car_point_distance(DEFAULT_CAR_POINT_DISTANCE), distance_buffer(DISTANCE_BUFFER), 
                                 is_fullscreen(false), windowed_width(WINDOW_WIDTH), windowed_height(WINDOW_HEIGHT), 
                                 windowed_pos_x(100), windowed_pos_y(100) {
    // Calculate scaling factors to fit the field in the window
    scale_x = (float)(WINDOW_WIDTH - 200) / FIELD_WIDTH;  // Leave space for UI
    scale_y = (float)(WINDOW_HEIGHT - 100) / FIELD_HEIGHT; // Leave space for UI
    
    // Use the smaller scale to maintain aspect ratio
    float scale = std::min(scale_x, scale_y);
    scale_x = scale_y = scale;
    
    // Calculate offset to center the field
    field_offset.x = (WINDOW_WIDTH - FIELD_WIDTH * scale_x) / 2;
    field_offset.y = 50; // Leave space at top for text
}

CarSimulation::~CarSimulation() {
    // Don't close window here, it's managed by main
}

void CarSimulation::initialize() {
    // Don't initialize window here, it's managed by main
    cars.resize(NUM_CARS);
}

void CarSimulation::updateFromDetectedObjects(const std::vector<DetectedObject>& detected_objects, const FieldTransform& field_transform) {
    // Clear previous points
    front_points.clear();
    identification_points.clear();
    
    // Convert detected objects to simulation points mit direkten Pixel-Koordinaten
    for (size_t i = 0; i < detected_objects.size(); i++) {
        const auto& obj = detected_objects[i];
        
        // Convert to window pixel coordinates for fullscreen display
        float window_x, window_y;
        cameraToWindow(obj, field_transform, window_x, window_y);
        
        if (obj.color == "Front") {
            front_points.emplace_back((int)window_x, (int)window_y, true, i);
        } else if (obj.color.find("Heck") == 0) {
            // Extract number from "Heck1", "Heck2", etc.
            int heck_id = 0;
            if (obj.color.length() > 4) {
                heck_id = obj.color[4] - '1'; // Convert "1", "2", etc. to 0, 1, etc.
            }
            identification_points.emplace_back((int)window_x, (int)window_y, false, heck_id);
        }
    }
}

// Helper function für Koordinatenumrechnung - gesamte Fläche = Crop-Bereich
void CarSimulation::cameraToWindow(const DetectedObject& obj, const FieldTransform& transform, float& window_x, float& window_y) {
    if (obj.crop_width <= 0 || obj.crop_height <= 0) {
        window_x = window_y = 0;
        return;
    }
    
    // Normalisiere Kamera-Koordinaten (0.0 bis 1.0)
    float norm_x = obj.coordinates.x / obj.crop_width;
    float norm_y = obj.coordinates.y / obj.crop_height;
    
    // Mappe direkt auf die gesamte Fensterfläche (gesamte Fläche = Crop-Bereich)
    window_x = norm_x * transform.field_width;
    window_y = norm_y * transform.field_height;
}

void CarSimulation::cameraToField(const DetectedObject& obj, const FieldTransform& transform, int& field_col, int& field_row) {
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
    field_col = std::max(0, std::min(field_col, transform.field_cols - 1));
    field_row = std::max(0, std::min(field_row, transform.field_rows - 1));
}

void CarSimulation::pairPointsToCars() {
    stablePairPointsToCars();
}

void CarSimulation::stablePairPointsToCars() {
    // First, check if existing stable pairs are still valid
    for (auto& car : cars) {
        if (car.front_point && car.identification_point && car.is_stable) {
            // Check if both points are still valid
            if (car.front_point->is_valid && car.identification_point->is_valid) {
                float current_distance = calculateDistance(*car.front_point, *car.identification_point);
                
                // Check if distance is within acceptable range (configurable distance +/- buffer)
                float min_distance = car_point_distance - distance_buffer;
                float max_distance = car_point_distance + distance_buffer;
                
                if (current_distance >= min_distance && current_distance <= max_distance) {
                    // Pair is still valid, update last known distance
                    car.last_distance = current_distance;
                    continue;
                }
            }
            
            // Pair is no longer valid, mark as unstable
            car.is_stable = false;
        }
    }
    
    // Track which points have been assigned to stable cars
    std::vector<bool> front_assigned(front_points.size(), false);
    std::vector<bool> id_assigned(identification_points.size(), false);
    
    // Mark points assigned to stable cars
    for (const auto& car : cars) {
        if (car.is_stable && car.front_point && car.identification_point) {
            for (size_t i = 0; i < front_points.size(); i++) {
                if (car.front_point == &front_points[i]) {
                    front_assigned[i] = true;
                    break;
                }
            }
            for (size_t i = 0; i < identification_points.size(); i++) {
                if (car.identification_point == &identification_points[i]) {
                    id_assigned[i] = true;
                    break;
                }
            }
        }
    }
    
    // Find new pairs for unstable cars
    for (auto& car : cars) {
        if (!car.is_stable) {
            float min_distance = std::numeric_limits<float>::max();
            int best_front = -1;
            int best_id = -1;
            
            // Find the closest unassigned valid points within distance range
            for (size_t f = 0; f < front_points.size(); f++) {
                if (front_assigned[f] || !front_points[f].is_valid) continue;
                
                for (size_t i = 0; i < identification_points.size(); i++) {
                    if (id_assigned[i] || !identification_points[i].is_valid) continue;
                    
                    float distance = calculateDistance(front_points[f], identification_points[i]);
                    
                    // Only consider pairs within the expected distance range
                    float min_expected = car_point_distance - distance_buffer;
                    float max_expected = car_point_distance + distance_buffer;
                    
                    if (distance >= min_expected && distance <= max_expected && distance < min_distance) {
                        min_distance = distance;
                        best_front = f;
                        best_id = i;
                    }
                }
            }
            
            // Assign the best pair to this car
            if (best_front != -1 && best_id != -1) {
                car.front_point = &front_points[best_front];
                car.identification_point = &identification_points[best_id];
                car.last_distance = min_distance;
                car.is_stable = true;
                car.car_id = identification_points[best_id].id;
                
                front_assigned[best_front] = true;
                id_assigned[best_id] = true;
            } else {
                // No suitable pair found, keep previous assignment if points are still valid
                if (car.front_point && !car.front_point->is_valid) {
                    car.front_point = nullptr;
                }
                if (car.identification_point && !car.identification_point->is_valid) {
                    car.identification_point = nullptr;
                }
            }
        }
    }
}

void CarSimulation::calculateCarDirections() {
    for (auto& car : cars) {
        if (car.front_point && car.identification_point) {
            car.direction = calculateAngle(*car.identification_point, *car.front_point);
        }
    }
}

float CarSimulation::calculateDistance(const Point& p1, const Point& p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return sqrt(dx * dx + dy * dy);
}

int CarSimulation::calculateAngle(const Point& from, const Point& to) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    
    // Calculate angle in radians, then convert to degrees
    float angle_rad = atan2(dx, -dy); // -dy because Y increases downward in screen coordinates
    float angle_deg = angle_rad * 180.0f / M_PI;
    
    // Normalize to 0-360 degrees
    if (angle_deg < 0) {
        angle_deg += 360.0f;
    }
    
    return (int)(angle_deg + 0.5f); // Round to nearest integer
}

void CarSimulation::update(float deltaTime) {
    time_elapsed += deltaTime;
    pairPointsToCars();
    calculateCarDirections();
}

void CarSimulation::renderField() {
    // Keine Spielfeld-Rahmen mehr - gesamte weiße Fläche = Crop-Bereich
    // Optional: Raster für Orientierung (sehr dezent)
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();
    
    // Sehr dezentes Raster alle 100 Pixel
    for (int x = 0; x < currentWidth; x += 100) {
        DrawLine(x, 0, x, currentHeight, LIGHTGRAY);
    }
    for (int y = 0; y < currentHeight; y += 100) {
        DrawLine(0, y, currentWidth, y, LIGHTGRAY);
    }
}

void CarSimulation::renderPoints() {
    // Render front points (große rote Kreise auf weißem Hintergrund)
    for (const auto& point : front_points) {
        if (point.is_valid) {
            Vector2 screen_pos = { (float)point.x, (float)point.y };
            DrawCircleV(screen_pos, 12, RED);
            DrawCircleLinesV(screen_pos, 12, MAROON);
            DrawText(TextFormat("FRONT-%d", point.id), screen_pos.x + 15, screen_pos.y - 15, 18, MAROON);
        }
    }
    
    // Render identification points (große blaue Quadrate auf weißem Hintergrund)
    for (const auto& point : identification_points) {
        if (point.is_valid) {
            Vector2 screen_pos = { (float)point.x, (float)point.y };
            DrawRectangle(screen_pos.x - 8, screen_pos.y - 8, 16, 16, BLUE);
            DrawRectangleLinesEx({screen_pos.x - 8, screen_pos.y - 8, 16, 16}, 2, DARKBLUE);
            DrawText(TextFormat("HECK-%d", point.id), screen_pos.x + 15, screen_pos.y - 15, 18, DARKBLUE);
        }
    }
}

void CarSimulation::renderCars() {
    // Render cars (connections between paired points) - auf weißem Hintergrund
    for (const auto& car : cars) {
        if (car.is_stable && car.front_point && car.identification_point && 
            car.front_point->is_valid && car.identification_point->is_valid) {
            
            // Direkte Pixel-Koordinaten verwenden
            Vector2 front_screen = { (float)car.front_point->x, (float)car.front_point->y };
            Vector2 id_screen = { (float)car.identification_point->x, (float)car.identification_point->y };
            
            // Zeichne Verbindungslinie (gut sichtbar auf weiß)
            DrawLineEx(id_screen, front_screen, 5, DARKGREEN);
            
            // Zeichne Richtungspfeil
            float arrow_length = 30;
            float angle_rad = car.direction * M_PI / 180.0f;
            Vector2 arrow_end = {
                (float)(front_screen.x + arrow_length * sin(angle_rad)),
                (float)(front_screen.y - arrow_length * cos(angle_rad))
            };
            
            DrawLineEx(front_screen, arrow_end, 6, ORANGE);
            
            // Zeichne Fahrzeug-Zentrum und Info
            Vector2 center = {
                (front_screen.x + id_screen.x) / 2,
                (front_screen.y + id_screen.y) / 2
            };
            
            DrawCircleV(center, 8, DARKGREEN);
            DrawText(TextFormat("CAR-%d: %d°", car.car_id, car.direction), 
                     center.x + 20, center.y - 20, 20, DARKGREEN);
        }
    }
}

void CarSimulation::renderUI() {
    // Komplett weißes Bild - KEINE schwarzen UI-Balken mehr!
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();
    
    // Koordinaten-Anzeige in den Ecken (schwarzer Text auf weißem Hintergrund)
    // Oben links: (0,0)
    DrawText("(0,0)", 5, 5, 20, BLACK);
    
    // Oben rechts: (max_x, 0)
    const char* top_right = TextFormat("(%d,0)", currentWidth-1);
    int tr_width = MeasureText(top_right, 20);
    DrawText(top_right, currentWidth - tr_width - 5, 5, 20, BLACK);
    
    // Unten links: (0, max_y)
    const char* bottom_left = TextFormat("(0,%d)", currentHeight-1);
    DrawText(bottom_left, 5, currentHeight - 25, 20, BLACK);
    
    // Unten rechts: (max_x, max_y)
    const char* bottom_right = TextFormat("(%d,%d)", currentWidth-1, currentHeight-1);
    int br_width = MeasureText(bottom_right, 20);
    DrawText(bottom_right, currentWidth - br_width - 5, currentHeight - 25, 20, BLACK);
    
    // Minimale Status-Info nur im Fenstermodus (nicht im Vollbild!)
    if (!is_fullscreen) {
        int stable_cars = 0;
        for (const auto& car : cars) {
            if (car.is_stable) stable_cars++;
        }
        
        // Sehr kleine Info in der Mitte oben (nur im Fenstermodus)
        const char* status = TextFormat("Autos: %d/%d | F11/F=Vollbild | V=Monitor2 | M=Verschieben | ESC=Ende", stable_cars, NUM_CARS);
        int status_width = MeasureText(status, 14);
        DrawText(status, (currentWidth - status_width) / 2, 5, 14, DARKGRAY);
    }
    // Im Vollbildmodus: KEINE störenden Texte - nur Koordinaten!
}

bool CarSimulation::shouldClose() {
    return WindowShouldClose();
}

void CarSimulation::toggleFullscreen() {
    if (is_fullscreen) {
        // Wechsel zu Fenstermodus
        setFullscreen(false);
    } else {
        // Wechsel zu Vollbildmodus
        setFullscreen(true);
    }
}

void CarSimulation::setFullscreen(bool fullscreen) {
    // Robuste Vollbild-Umschaltung mit mehreren Methoden
    bool currently_fullscreen = IsWindowFullscreen();
    
    if (fullscreen && !currently_fullscreen) {
        // Speichere aktuelle Fensterposition und -größe
        windowed_pos_x = GetWindowPosition().x;
        windowed_pos_y = GetWindowPosition().y;
        windowed_width = GetScreenWidth();
        windowed_height = GetScreenHeight();
        
        printf("=== VOLLBILD AKTIVIERUNG ===\n");
        printf("Von: %dx%d bei (%d,%d)\n", windowed_width, windowed_height, windowed_pos_x, windowed_pos_y);
        
        // Methode 1: Direkter ToggleFullscreen
        printf("Versuche Methode 1: ToggleFullscreen...\n");
        ToggleFullscreen();
        
        // Prüfe ob es funktioniert hat
        if (IsWindowFullscreen()) {
            is_fullscreen = true;
            printf("✅ Vollbild aktiviert! Neue Größe: %dx%d\n", GetScreenWidth(), GetScreenHeight());
        } else {
            printf("❌ Methode 1 fehlgeschlagen. Versuche Methode 2...\n");
            
            // Methode 2: Manuell auf Monitor-Größe setzen
            int monitor = GetCurrentMonitor();
            int monitor_width = GetMonitorWidth(monitor);
            int monitor_height = GetMonitorHeight(monitor);
            Vector2 monitor_pos = GetMonitorPosition(monitor);
            
            printf("Monitor %d: %dx%d bei (%.0f,%.0f)\n", monitor, monitor_width, monitor_height, monitor_pos.x, monitor_pos.y);
            
            // Entferne Fensterdekoration und setze auf Monitorgröße
            SetWindowState(FLAG_WINDOW_UNDECORATED);
            SetWindowSize(monitor_width, monitor_height);
            SetWindowPosition((int)monitor_pos.x, (int)monitor_pos.y);
            
            is_fullscreen = true;
            printf("✅ Pseudo-Vollbild aktiviert: %dx%d\n", GetScreenWidth(), GetScreenHeight());
        }
        
    } else if (!fullscreen && currently_fullscreen) {
        printf("=== FENSTERMODUS AKTIVIERUNG ===\n");
        
        // Wenn echter Vollbildmodus
        if (IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        
        // Fensterdekoration wiederherstellen
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
        
        // Ursprüngliche Größe wiederherstellen
        SetWindowSize(windowed_width, windowed_height);
        SetWindowPosition(windowed_pos_x, windowed_pos_y);
        
        is_fullscreen = false;
        printf("✅ Fenstermodus wiederhergestellt: %dx%d bei (%d,%d)\n", 
               windowed_width, windowed_height, windowed_pos_x, windowed_pos_y);
    } else {
        printf("Vollbild-Status bereits korrekt: %s\n", currently_fullscreen ? "Vollbild" : "Fenster");
    }
}

void CarSimulation::updateFieldTransformForCurrentScreen(FieldTransform& transform) {
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();
    
    // Update transform für aktuellen Bildschirm - GESAMTE Fensterfläche nutzen
    transform.field_width = currentWidth;
    transform.field_height = currentHeight;
    transform.offset_x = 0;
    transform.offset_y = 0;
    
    // Keine UI-Bereiche mehr abziehen - komplettes weißes Fenster!
}
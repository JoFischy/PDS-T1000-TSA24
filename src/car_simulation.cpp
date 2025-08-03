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

CarSimulation::CarSimulation() : time_elapsed(0.0f), car_point_distance(DEFAULT_CAR_POINT_DISTANCE), distance_buffer(DISTANCE_BUFFER) {
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
    
    // Convert detected objects to simulation points
    for (size_t i = 0; i < detected_objects.size(); i++) {
        const auto& obj = detected_objects[i];
        
        // Convert to field coordinates
        int field_col, field_row;
        cameraToField(obj, field_transform, field_col, field_row);
        
        if (obj.color == "Front") {
            front_points.emplace_back(field_col, field_row, true, i);
        } else if (obj.color.find("Heck") == 0) {
            // Extract number from "Heck1", "Heck2", etc.
            int heck_id = 0;
            if (obj.color.length() > 4) {
                heck_id = obj.color[4] - '1'; // Convert "1", "2", etc. to 0, 1, etc.
            }
            identification_points.emplace_back(field_col, field_row, false, heck_id);
        }
    }
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
    // Draw field boundary
    Rectangle field_rect = {
        field_offset.x,
        field_offset.y,
        FIELD_WIDTH * scale_x,
        FIELD_HEIGHT * scale_y
    };
    
    DrawRectangleRec(field_rect, LIGHTGRAY);
    DrawRectangleLinesEx(field_rect, 2, BLACK);
    
    // Draw field dimensions
    DrawText(TextFormat("Field: %dx%d", FIELD_WIDTH, FIELD_HEIGHT), 
             field_offset.x, field_offset.y - 25, 20, BLACK);
}

void CarSimulation::renderPoints() {
    // Render front points (red circles)
    for (const auto& point : front_points) {
        if (point.is_valid) {
            Vector2 screen_pos = {
                field_offset.x + point.x * scale_x,
                field_offset.y + point.y * scale_y
            };
            DrawCircleV(screen_pos, 4, RED);
            DrawText(TextFormat("F%d", point.id), screen_pos.x + 6, screen_pos.y - 6, 12, RED);
        }
    }
    
    // Render identification points (blue squares)
    for (const auto& point : identification_points) {
        if (point.is_valid) {
            Vector2 screen_pos = {
                field_offset.x + point.x * scale_x,
                field_offset.y + point.y * scale_y
            };
            DrawRectangle(screen_pos.x - 3, screen_pos.y - 3, 6, 6, BLUE);
            DrawText(TextFormat("I%d", point.id), screen_pos.x + 6, screen_pos.y - 6, 12, BLUE);
        }
    }
}

void CarSimulation::renderCars() {
    // Draw lines connecting paired points
    for (const auto& car : cars) {
        if (car.front_point && car.identification_point && car.is_stable) {
            Vector2 front_screen = {
                field_offset.x + car.front_point->x * scale_x,
                field_offset.y + car.front_point->y * scale_y
            };
            Vector2 id_screen = {
                field_offset.x + car.identification_point->x * scale_x,
                field_offset.y + car.identification_point->y * scale_y
            };
            
            // Draw connection line
            DrawLineEx(id_screen, front_screen, 2, GREEN);
            
            // Draw direction arrow
            float arrow_length = 15;
            float angle_rad = car.direction * M_PI / 180.0f;
            Vector2 arrow_end = {
                (float)(front_screen.x + arrow_length * sin(angle_rad)),
                (float)(front_screen.y - arrow_length * cos(angle_rad))
            };
            
            DrawLineEx(front_screen, arrow_end, 3, ORANGE);
            
            // Draw car center and info
            Vector2 center = {
                (front_screen.x + id_screen.x) / 2,
                (front_screen.y + id_screen.y) / 2
            };
            
            DrawCircleV(center, 3, GREEN);
            DrawText(TextFormat("Car%d: %d°", car.car_id, car.direction), 
                     center.x + 8, center.y - 8, 10, GREEN);
        }
    }
}

void CarSimulation::renderUI() {
    // Status information
    int stable_cars = 0;
    for (const auto& car : cars) {
        if (car.is_stable) stable_cars++;
    }
    
    DrawText(TextFormat("Cars detected: %d/%d", stable_cars, NUM_CARS), 10, 10, 20, GREEN);
    DrawText(TextFormat("Front points: %d", (int)front_points.size()), 10, 35, 16, RED);
    DrawText(TextFormat("ID points: %d", (int)identification_points.size()), 10, 55, 16, BLUE);
    DrawText(TextFormat("Distance: %.1f (±%.1f)", car_point_distance, distance_buffer), 10, 75, 16, BLACK);
}

bool CarSimulation::shouldClose() {
    return WindowShouldClose();
}
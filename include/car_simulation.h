#ifndef CAR_SIMULATION_H
#define CAR_SIMULATION_H

#include "raylib.h"
#include <vector>
#include <cmath>

// Forward declaration for DetectedObject from Vehicle.h
struct DetectedObject;

// Constants für Vollbild-Modus
const int FIELD_WIDTH = 182;
const int FIELD_HEIGHT = 93;
const int WINDOW_WIDTH = 1920;  // Vollbild-Breite (wird dynamisch angepasst)
const int WINDOW_HEIGHT = 1080; // Vollbild-Höhe (wird dynamisch angepasst)
const int NUM_CARS = 4;
const float DEFAULT_CAR_POINT_DISTANCE = 10.0f; // Default distance between front and identification points
const float DISTANCE_BUFFER = 3.0f; // Buffer for distance matching
const float PAIRING_STABILITY_THRESHOLD = 20.0f; // Minimum distance before reassigning pairs
const int FIELD_SIZE = 15;       // Größere Felder für Vollbild
const int UI_HEIGHT = 100;       // Größere UI für Vollbild

// Spielfeld-zu-Fenster Transformation Structure
struct FieldTransform {
    int field_cols, field_rows;     // Anzahl Spalten und Zeilen
    int field_width, field_height;  // Spielfeld-Dimensionen in virtuellen Einheiten
    float offset_x, offset_y;       // Zentrierung
    
    void calculate(int window_width, int window_height);
};

// Point structure
struct Point {
    int x, y; // Changed to int for whole numbers
    bool is_front_point;
    int id;
    Color color;
    bool is_valid; // Track if point coordinates are available
    
    Point(int x = 0, int y = 0, bool is_front = false, int id = 0) 
        : x(x), y(y), is_front_point(is_front), id(id), is_valid(true) {
        color = is_front_point ? RED : BLUE;
    }
};

// Car structure
struct Car {
    Point* front_point;
    Point* identification_point;
    int direction; // in degrees (0° = up, 90° = right, 180° = down, 270° = left) - changed to int
    int car_id;
    bool is_stable; // Track if this car assignment is stable
    float last_distance; // Last known distance between points for stability checking
    
    Car() : front_point(nullptr), identification_point(nullptr), direction(0), car_id(-1), is_stable(false), last_distance(0.0f) {}
};

// CarSimulation class
class CarSimulation {
private:
    std::vector<Point> front_points;
    std::vector<Point> identification_points;
    std::vector<Car> cars;
    
    // Simulation parameters
    float time_elapsed;
    float scale_x, scale_y; // Scaling factors for rendering
    Vector2 field_offset;   // Offset for centering the field
    float car_point_distance; // Configurable distance between car points
    float distance_buffer;   // Buffer for distance matching
    
    // Fullscreen management
    bool is_fullscreen;
    int windowed_width, windowed_height;
    int windowed_pos_x, windowed_pos_y;
    
    // Private methods
    void pairPointsToCars();
    void stablePairPointsToCars(); // New stable pairing method
    void calculateCarDirections();
    float calculateDistance(const Point& p1, const Point& p2);
    int calculateAngle(const Point& from, const Point& to); // Changed to return int
    
    // Helper functions for coordinate conversion
    void cameraToField(const DetectedObject& obj, const FieldTransform& transform, int& field_col, int& field_row);
    void cameraToWindow(const DetectedObject& obj, const FieldTransform& transform, float& window_x, float& window_y);
    
public:
    CarSimulation();
    ~CarSimulation();
    
    void initialize();
    void update(float deltaTime);
    void updateFromDetectedObjects(const std::vector<DetectedObject>& detected_objects, const FieldTransform& field_transform);
    bool shouldClose();
    
    // Rendering methods
    void renderField();
    void renderPoints();
    void renderCars();
    void renderUI();
    
    // Configuration methods
    void setCarPointDistance(float distance) { car_point_distance = distance; }
    void setDistanceBuffer(float buffer) { distance_buffer = buffer; }
    float getCarPointDistance() const { return car_point_distance; }
    float getDistanceBuffer() const { return distance_buffer; }
    
    // Fullscreen management methods
    void toggleFullscreen();
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const { return is_fullscreen; }
    void updateFieldTransformForCurrentScreen(FieldTransform& transform);
    
    // Getters for external access
    const std::vector<Car>& getCars() const { return cars; }
    Vector2 getFieldSize() const { return {FIELD_WIDTH, FIELD_HEIGHT}; }
};

#endif // CAR_SIMULATION_H
#ifndef CAR_SIMULATION_H
#define CAR_SIMULATION_H

#include "raylib.h"
#include "Vehicle.h" // Assuming this is for DetectedObject
#include "point.h"
#include "auto.h"
#include <vector>
#include <memory> // Not strictly used in the provided snippet, but good to keep if intended for future use.

// Forward declaration for Renderer
class Renderer;

// Constants for the simulation
const int FIELD_SIZE = 10;       // Smaller field size for the new system
const int FIELD_WIDTH = 120;     // New field width
const int FIELD_HEIGHT = 80;     // New field height
const int WINDOW_WIDTH = 1200;   // New window width for full-screen layout
const int WINDOW_HEIGHT = 800;   // New window height for full-screen layout
const int NUM_CARS = 4;          // Number of cars to simulate

// Structure to handle field-to-window transformations
struct FieldTransform {
    int field_cols;
    int field_rows;
    int field_width;
    int field_height;
    int offset_x;
    int offset_y;

    // Method to calculate transformation parameters
    void calculate(int window_width, int window_height);
};

// Main class for car simulation
class CarSimulation {
private:
    std::vector<Point> points;             // Stores the processed points
    std::vector<Auto> detectedAutos;       // Stores the detected vehicles as Autos
    Renderer* renderer;                    // Pointer to the renderer
    float tolerance;                       // Tolerance for point matching
    float time_elapsed;                    // Time elapsed in the simulation
    float car_point_distance;              // Configurable distance between car points
    float distance_buffer;                 // Buffer for distance matching

public:
    // Constructor and Destructor
    CarSimulation();
    ~CarSimulation();

    // Initialization method
    void initialize();

    // Update methods
    // Updates the simulation based on detected objects from camera
    void updateFromDetectedObjects(const std::vector<DetectedObject>& detected_objects, const FieldTransform& field_transform);
    // General update method for simulation logic
    void update(float deltaTime);

    // Rendering methods
    void renderField();
    void renderPoints();
    void renderCars();
    void renderUI();

    // Configuration methods
    void setCarPointDistance(float distance);
    void setDistanceBuffer(float buffer);

private:
    // Private helper methods for simulation logic
    void detectVehicles(); // Logic to detect vehicles from points
    // Converts camera coordinates to window coordinates
    void cameraToWindow(const DetectedObject& obj, const FieldTransform& transform, float& window_x, float& window_y);
};

#endif // CAR_SIMULATION_H
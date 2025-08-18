#ifndef CAR_SIMULATION_H
#define CAR_SIMULATION_H

#include "raylib.h"
#include "Vehicle.h"
#include "point.h"
#include "auto.h"
#include "coordinate_filter.h"
#include "path_system.h"
#include "segment_manager.h"
#include "vehicle_controller.h"
#include <vector>
#include <memory>

// Forward declaration for Renderer
class Renderer;

// Constants for the simulation
const int FIELD_SIZE = 10;
const int FIELD_WIDTH = 120;
const int FIELD_HEIGHT = 80;
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const int NUM_CARS = 4;

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
    std::vector<Point> points;
    std::vector<Auto> detectedAutos;
    Renderer* renderer;
    float tolerance;
    float time_elapsed;
    float car_point_distance;
    float distance_buffer;
    std::unique_ptr<CoordinateFilter> fastFilter;

    // New path system components
    PathSystem pathSystem;
    SegmentManager* segmentManager;
    VehicleController* vehicleController;
    bool pathSystemInitialized;

    // Input handling
    int selectedVehicle;
    void handleVehicleSelection();
    void handleTargetAssignment();

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

    // New path system methods
    void initializePathSystem();
    void createFactoryPathSystem();
    void syncDetectedVehiclesWithPathSystem();
    int mapDetectedVehicleToPathSystem(const Auto& detectedAuto);

private:
    // Private helper methods for simulation logic
    void detectVehicles(); // Logic to detect vehicles from points
    // Converts camera coordinates to window coordinates
    void cameraToWindow(const DetectedObject& obj, const FieldTransform& transform, float& window_x, float& window_y);
    
    // Path system helpers
    Point transformToPathSystemCoordinates(const Point& detectedPosition, const FieldTransform& transform);
    void updateVehicleFromDetection(int vehicleId, const Auto& detectedAuto, const FieldTransform& transform);
};

#endif // CAR_SIMULATION_H
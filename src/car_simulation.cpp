#include "car_simulation.h"
#include "point.h"
#include "auto.h"
#include "renderer.h"
#include "py_runner.h"
#include "coordinate_filter.h"
#include <algorithm>
#include <cmath>

#define DEFAULT_CAR_POINT_DISTANCE 12.0f
#define DISTANCE_BUFFER 4.0f

CarSimulation::CarSimulation() : tolerance(100.0f), time_elapsed(0.0f), car_point_distance(DEFAULT_CAR_POINT_DISTANCE), 
                                 distance_buffer(DISTANCE_BUFFER),
                                 coordinateFilter(50.0f, 5.0f, 2, 8, 300.0f) {
    renderer = nullptr;
}

CarSimulation::~CarSimulation() {
    if (renderer) {
        delete renderer;
    }
}

void CarSimulation::initialize() {
    // Initialize renderer with current screen size
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();
    renderer = new Renderer(currentWidth, currentHeight);
    renderer->initialize();
}

void CarSimulation::updateFromDetectedObjects(const std::vector<DetectedObject>& detected_objects, const FieldTransform& field_transform) {
    // Convert detected objects to Points with window coordinates
    std::vector<Point> rawPoints;
    std::vector<std::string> colors;

    for (size_t i = 0; i < detected_objects.size(); i++) {
        const auto& obj = detected_objects[i];

        // Convert to window pixel coordinates for fullscreen display
        float window_x, window_y;
        cameraToWindow(obj, field_transform, window_x, window_y);

        if (obj.color == "Front") {
            rawPoints.emplace_back(window_x, window_y, PointType::FRONT, obj.color);
            colors.push_back(obj.color);
        } else if (obj.color.find("Heck") == 0) {
            rawPoints.emplace_back(window_x, window_y, PointType::IDENTIFICATION, obj.color);
            colors.push_back(obj.color);
        }
    }

    // Filter points through coordinate filter to get stable points only
    points = coordinateFilter.filterAndSmooth(rawPoints, colors);

    // Detect vehicles from filtered points only
    detectVehicles();
}

void CarSimulation::detectVehicles() {
    detectedAutos.clear();

    // Create vectors to separate identification and front points
    std::vector<size_t> identificationIndices;
    std::vector<size_t> frontIndices;

    for (size_t i = 0; i < points.size(); i++) {
        if (points[i].type == PointType::IDENTIFICATION) {
            identificationIndices.push_back(i);
        } else if (points[i].type == PointType::FRONT) {
            frontIndices.push_back(i);
        }
    }

    // Track which front points have already been used
    std::vector<bool> frontPointUsed(frontIndices.size(), false);

    // For each identification point, find the closest available front point within tolerance
    for (size_t idIdx : identificationIndices) {
        float bestDistance = tolerance + 1.0f;  // Start with distance greater than tolerance
        int bestFrontIdx = -1;
        int bestFrontVectorIdx = -1;

        // Check all front points
        for (size_t j = 0; j < frontIndices.size(); j++) {
            if (frontPointUsed[j]) continue;  // Skip already used front points

            size_t frontIdx = frontIndices[j];
            float distance = points[idIdx].distanceTo(points[frontIdx]);

            // If this front point is closer and within tolerance
            if (distance <= tolerance && distance < bestDistance) {
                bestDistance = distance;
                bestFrontIdx = frontIdx;
                bestFrontVectorIdx = j;
            }
        }

        // If we found a suitable front point, create a vehicle
        if (bestFrontIdx != -1) {
            detectedAutos.emplace_back(points[idIdx], points[bestFrontIdx]);
            frontPointUsed[bestFrontVectorIdx] = true;  // Mark this front point as used
        }
    }
}

void CarSimulation::update(float deltaTime) {
    time_elapsed += deltaTime;

    // Update tolerance with +/- keys for debugging
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
        tolerance += 10.0f;
        if (tolerance > 300.0f) tolerance = 300.0f;
    }

    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        tolerance -= 10.0f;
        if (tolerance < 10.0f) tolerance = 10.0f;
    }
}

void CarSimulation::renderPoints() {
    // Points are rendered by the renderer
}

void CarSimulation::renderCars() {
    // Cars are rendered by the renderer
}

void CarSimulation::renderUI() {
    // UI is rendered by the renderer
    if (renderer) {
        renderer->render(points, detectedAutos, tolerance);
    }
}

void CarSimulation::renderField() {
    // Optional: Very subtle grid for orientation
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();

    for (int x = 0; x < currentWidth; x += 100) {
        DrawLine(x, 0, x, currentHeight, LIGHTGRAY);
    }
    for (int y = 0; y < currentHeight; y += 100) {
        DrawLine(0, y, currentWidth, y, LIGHTGRAY);
    }
}

void CarSimulation::cameraToWindow(const DetectedObject& obj, const FieldTransform& transform, float& window_x, float& window_y) {
    if (obj.crop_width <= 0 || obj.crop_height <= 0) {
        window_x = window_y = 0;
        return;
    }

    // Normalize camera coordinates (0.0 to 1.0)
    float norm_x = obj.coordinates.x / obj.crop_width;
    float norm_y = obj.coordinates.y / obj.crop_height;

    // Map directly to entire window area (entire area = crop area)
    window_x = norm_x * transform.field_width;
    window_y = norm_y * transform.field_height;
}

void CarSimulation::setCarPointDistance(float distance) {
    car_point_distance = distance;
}

void CarSimulation::setDistanceBuffer(float buffer) {
    distance_buffer = buffer;
}

// Implementation of FieldTransform::calculate method
void FieldTransform::calculate(int window_width, int window_height) {
    // UTILIZE THE ENTIRE WINDOW AREA - NO MORE UI_HEIGHT SUBTRACTION!
    int available_width = window_width;
    int available_height = window_height;  // Completely without UI subtraction

    // Calculate the number of square fields that fit
    field_cols = available_width / FIELD_SIZE;
    field_rows = available_height / FIELD_SIZE;

    // Actual field dimensions = ENTIRE window area
    field_width = window_width;   // Complete width
    field_height = window_height; // Complete height

    // No offset - use the entire area
    offset_x = 0;
    offset_y = 0;
}
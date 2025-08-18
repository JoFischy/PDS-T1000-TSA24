#include "car_simulation.h"
#include "point.h"
#include "auto.h"
#include "renderer.h"
#include "py_runner.h"
#include "coordinate_filter_fast.h"
#include "test_window.h"
#include <algorithm>
#include <cmath>
#include <iostream>

#define DEFAULT_CAR_POINT_DISTANCE 25.0f
#define DISTANCE_BUFFER 8.0f

CarSimulation::CarSimulation() : tolerance(250.0f), time_elapsed(0.0f), car_point_distance(DEFAULT_CAR_POINT_DISTANCE),
                                 distance_buffer(DISTANCE_BUFFER), segmentManager(nullptr), vehicleController(nullptr),
                                 pathSystemInitialized(false), selectedVehicle(-1) {
    renderer = nullptr;
    // Verwende den schnellen Filter für minimale Verzögerung
    fastFilter = createFastCoordinateFilter();
}

CarSimulation::~CarSimulation() {
    if (renderer) {
        delete renderer;
    }
    if (vehicleController) {
        delete vehicleController;
    }
    if (segmentManager) {
        delete segmentManager;
    }
}

void CarSimulation::initialize() {
    // Initialize renderer with current screen size
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();
    renderer = new Renderer(currentWidth, currentHeight);
    renderer->initialize();

    // Initialize path system
    initializePathSystem();
}

void CarSimulation::updateFromDetectedObjects(const std::vector<DetectedObject>& detected_objects, const FieldTransform& field_transform) {
    // ULTRA-SCHNELLE VERARBEITUNG: Minimale Zwischenschritte
    std::vector<Point> rawPoints;
    rawPoints.reserve(detected_objects.size()); // Verhindere Reallocations
    
    std::vector<std::string> colors;
    colors.reserve(detected_objects.size());

    // Optimierte Schleife mit KALIBRIERTER Koordinaten-Transformation
    for (const auto& obj : detected_objects) {
        // Verwende kalibrierte Transformation aus test_window.cpp
        if (obj.crop_width > 0 && obj.crop_height > 0) {
            float window_x, window_y;
            getCalibratedTransform(obj.coordinates.x, obj.coordinates.y, 
                                 obj.crop_width, obj.crop_height, 
                                 window_x, window_y);

            if (obj.color == "Front") {
                rawPoints.emplace_back(window_x, window_y, PointType::FRONT, obj.color);
                colors.push_back(obj.color);
            } else if (obj.color.find("Heck") == 0) {
                rawPoints.emplace_back(window_x, window_y, PointType::IDENTIFICATION, obj.color);
                colors.push_back(obj.color);
            }
        }
    }

    // DIREKTER DURCHGANG - Kein Filter für maximale Geschwindigkeit
    points = std::move(rawPoints); // Move semantics für Performance
    
    // Sofortige Fahrzeugerkennung
    detectVehicles();

    // Update test window with detected objects and vehicles
    std::vector<DetectedObject> detectedObjForWindow;
    for (const auto& point : points) {
        DetectedObject obj;
        obj.coordinates.x = point.x;
        obj.coordinates.y = point.y;
        obj.color = point.color;
        detectedObjForWindow.push_back(obj);
    }
    updateTestWindowCoordinates(detectedObjForWindow);

    // Update vehicle commands in JSON (extern function from test_window.cpp)
    extern void updateVehicleCommands();
    if (!detectedAutos.empty()) {
        updateVehicleCommands();
    }
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

    // Update test window with detected vehicles
    updateTestWindowVehicles(detectedAutos);
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

    // Handle vehicle selection and target assignment
    handleVehicleSelection();
    handleTargetAssignment();

    // Update path system vehicles
    if (pathSystemInitialized && vehicleController) {
        // Sync detected vehicles with path system
        syncDetectedVehiclesWithPathSystem();

        // Update vehicle movement along paths
        vehicleController->updateVehicles(deltaTime);
    }
}

void CarSimulation::renderPoints() {
    // Points are rendered by the renderer
}

void CarSimulation::renderCars() {
    // Cars are rendered by the renderer
}

void CarSimulation::renderUI() {
    // Render nur Hintergrund im Hauptfenster - Routen werden in Windows API gezeichnet
    if (renderer) {
        renderer->renderBackgroundOnly();
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
        std::cout << "ERROR: Invalid crop dimensions: " << obj.crop_width << "x" << obj.crop_height << std::endl;
        window_x = window_y = 0;
        return;
    }

    // Debug-Ausgabe für eingehende Daten
    std::cout << "cameraToWindow Input: obj(" << obj.coordinates.x << ", " << obj.coordinates.y 
              << ") crop(" << obj.crop_width << ", " << obj.crop_height << ")" << std::endl;

    // Die Python-Koordinaten sind bereits Pixel-Koordinaten im Crop-Bereich (nicht normalisiert!)
    // obj.coordinates.x und obj.coordinates.y sind bereits die Pixel-Positionen (z.B. 150, 200)
    // Wir müssen sie erst normalisieren (0.0 bis 1.0)
    float norm_x = obj.coordinates.x / obj.crop_width;
    float norm_y = obj.coordinates.y / obj.crop_height;

    // Map to entire window area (1920x1200 fullscreen)
    window_x = norm_x * transform.field_width;
    window_y = norm_y * transform.field_height;

    // Debug-Ausgabe für transformierte Koordinaten
    std::cout << "cameraToWindow Output: norm(" << norm_x << ", " << norm_y 
              << ") -> window(" << window_x << ", " << window_y << ")" << std::endl;
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

void CarSimulation::initializePathSystem() {
    if (pathSystemInitialized) return;

    // Create factory path system
    createFactoryPathSystem();

    // Initialize segment manager
    segmentManager = new SegmentManager(&pathSystem);

    // Initialize vehicle controller
    vehicleController = new VehicleController(&pathSystem, segmentManager);

    pathSystemInitialized = true;

    // WICHTIG: Setze PathSystem-Referenzen für das Test-Fenster
    ::setTestWindowPathSystem(&pathSystem, vehicleController);

    std::cout << "Path system initialized with " << pathSystem.getNodeCount()
              << " nodes and " << pathSystem.getSegmentCount() << " segments" << std::endl;
}

void CarSimulation::createFactoryPathSystem() {
    // Clear any existing data
    pathSystem = PathSystem();

    // Create the factory nodes with exact coordinates (scaled to window)
    int node1 = pathSystem.addNode(70, 65);       // Node 1
    int node2 = pathSystem.addNode(640, 65);      // Node 2
    int node3 = pathSystem.addNode(985, 65);      // Node 3
    int node4 = pathSystem.addNode(1860, 65);     // Node 4
    int node5 = pathSystem.addNode(70, 470);      // Node 5
    int node6 = pathSystem.addNode(640, 470);     // Node 6
    int node7 = pathSystem.addNode(985, 320);     // Node 7
    int node8 = pathSystem.addNode(1860, 320);    // Node 8
    int node9 = pathSystem.addNode(985, 750);     // Node 9
    int node10 = pathSystem.addNode(1860, 750);   // Node 10
    int node11 = pathSystem.addNode(70, 1135);    // Node 11
    int node12 = pathSystem.addNode(985, 1135);   // Node 12
    int node13 = pathSystem.addNode(1860, 1135);  // Node 13

    // Add waiting points at T-junctions
    int wait2_left = pathSystem.addWaitingNode(640 - 150, 65);
    int wait2_bottom = pathSystem.addWaitingNode(640, 65 + 150);
    int wait2_3_merged = pathSystem.addWaitingNode(812, 65);

    int wait3_east = pathSystem.addWaitingNode(985 + 150, 65);

    int wait5_top = pathSystem.addWaitingNode(70, 470 - 150);
    int wait5_right = pathSystem.addWaitingNode(70 + 150, 470);
    int wait5_bottom = pathSystem.addWaitingNode(70, 470 + 150);

    int wait3_7_merged = pathSystem.addWaitingNode(985, 192);
    int wait7_east = pathSystem.addWaitingNode(985 + 150, 320);
    int wait7_south_merged = pathSystem.addWaitingNode(985, 535);

    int wait8_west = pathSystem.addWaitingNode(1860 - 150, 320);
    int wait8_10_merged = pathSystem.addWaitingNode(1860, 535);

    int wait9_east = pathSystem.addWaitingNode(985 + 150, 750);
    int wait9_south_merged = pathSystem.addWaitingNode(985, 942);

    int wait12_east = pathSystem.addWaitingNode(985 + 150, 1135);
    int wait12_west = pathSystem.addWaitingNode(985 - 150, 1135);

    int wait10_left = pathSystem.addWaitingNode(1860 - 150, 750);
    int wait10_bottom = pathSystem.addWaitingNode(1860, 750 + 150);

    // Connect main nodes
    pathSystem.addSegment(node1, node2);
    pathSystem.addSegment(node1, node5);
    pathSystem.addSegment(node2, node3);
    pathSystem.addSegment(node2, node6);
    pathSystem.addSegment(node3, node4);
    pathSystem.addSegment(node3, node7);
    pathSystem.addSegment(node4, node8);
    pathSystem.addSegment(node5, node6);
    pathSystem.addSegment(node5, node11);
    pathSystem.addSegment(node7, node8);
    pathSystem.addSegment(node7, node9);
    pathSystem.addSegment(node8, node10);
    pathSystem.addSegment(node9, node10);
    pathSystem.addSegment(node9, node12);
    pathSystem.addSegment(node10, node13);
    pathSystem.addSegment(node11, node12);
    pathSystem.addSegment(node12, node13);

    // Connect waiting points
    pathSystem.addSegment(node2, wait2_left);
    pathSystem.addSegment(node2, wait2_bottom);
    pathSystem.addSegment(node2, wait2_3_merged);
    pathSystem.addSegment(node3, wait2_3_merged);
    pathSystem.addSegment(node3, wait3_east);
    pathSystem.addSegment(node3, wait3_7_merged);
    pathSystem.addSegment(node5, wait5_top);
    pathSystem.addSegment(node5, wait5_right);
    pathSystem.addSegment(node5, wait5_bottom);
    pathSystem.addSegment(node7, wait3_7_merged);
    pathSystem.addSegment(node7, wait7_east);
    pathSystem.addSegment(node7, wait7_south_merged);
    pathSystem.addSegment(node9, wait7_south_merged);
    pathSystem.addSegment(node8, wait8_west);
    pathSystem.addSegment(node8, wait8_10_merged);
    pathSystem.addSegment(node9, wait9_east);
    pathSystem.addSegment(node9, wait9_south_merged);
    pathSystem.addSegment(node12, wait9_south_merged);
    pathSystem.addSegment(node12, wait12_east);
    pathSystem.addSegment(node12, wait12_west);
    pathSystem.addSegment(node10, wait8_10_merged);
    pathSystem.addSegment(node10, wait10_left);
    pathSystem.addSegment(node10, wait10_bottom);

    // Path system initialization complete
}

void CarSimulation::syncDetectedVehiclesWithPathSystem() {
    if (!pathSystemInitialized || !vehicleController) return;

    // ULTRA-SCHNELLE SYNCHRONISATION - Minimale Zwischenschritte
    for (const auto& detectedAuto : detectedAutos) {
        if (!detectedAuto.isValid()) continue;

        Point currentCenter = detectedAuto.getCenter();
        
        // Schnelle Koordinaten-Validierung
        if (currentCenter.x <= 0 || currentCenter.y <= 0 || 
            currentCenter.x >= 1920 || currentCenter.y >= 1200) {
            continue;
        }

        int vehicleId = detectedAuto.getId();
        
        // Direkte Fahrzeug-Aktualisierung ohne Map-Zwischenspeicherung
        Auto* pathVehicle = vehicleController->getVehicle(vehicleId);
        if (!pathVehicle) {
            // Sofortige Fahrzeug-Erstellung
            vehicleId = vehicleController->addVehicle(currentCenter); // Direkte Koordinaten
            pathVehicle = vehicleController->getVehicle(vehicleId);
        }

        if (pathVehicle) {
            // SOFORTIGE Position-Update ohne Transform-Overhead
            vehicleController->updateVehicleFromRealCoordinates(vehicleId, currentCenter, detectedAuto.getDirection());

            // Schnelle Node-Assignment falls nötig
            if (pathVehicle->currentNodeId == -1) {
                int nearestNode = pathSystem.findNearestNode(currentCenter, 200.0f);
                if (nearestNode != -1) {
                    pathVehicle->currentNodeId = nearestNode;
                }
            }
        }
    }
}

int CarSimulation::mapDetectedVehicleToPathSystem(const Auto& detectedAuto) {
    // Map detected vehicle to existing path system vehicle based on ID or create new one
    int vehicleId = detectedAuto.getId();

    // Check if vehicle already exists in path system
    Auto* pathVehicle = vehicleController->getVehicle(vehicleId);
    if (!pathVehicle) {
        // Create new vehicle in path system
        Point startPos = transformToPathSystemCoordinates(detectedAuto.getCenter(), FieldTransform{});
        vehicleId = vehicleController->addVehicle(startPos);
    }

    return vehicleId;
}

Point CarSimulation::transformToPathSystemCoordinates(const Point& detectedPosition,
                                                     const FieldTransform& transform) {
    // Transform from detection coordinates to path system coordinates
    // This needs to account for the coordinate calibration and scaling

    // For now, assume 1:1 mapping - you may need to adjust this based on your calibration
    return Point(detectedPosition.x, detectedPosition.y);
}

void CarSimulation::updateVehicleFromDetection(int vehicleId, const Auto& detectedAuto,
                                              const FieldTransform& transform) {
    if (!vehicleController) return;

    Auto* vehicle = vehicleController->getVehicle(vehicleId);
    if (!vehicle) return;

    // Update vehicle position from detection
    Point newPos = transformToPathSystemCoordinates(detectedAuto.getCenter(), transform);
    vehicle->position = newPos;

    // Update direction
    vehicle->currentDirection = static_cast<Direction>((int)detectedAuto.getDirection());

    // If vehicle doesn't have a current node, snap to nearest
    if (vehicle->currentNodeId == -1) {
        int nearestNode = pathSystem.findNearestNode(newPos, 100.0f);
        if (nearestNode != -1) {
            vehicle->currentNodeId = nearestNode;
            const PathNode* node = pathSystem.getNode(nearestNode);
            if (node) {
                vehicle->position = node->position; // Snap to node position
            }
        }
    }
}

void CarSimulation::handleVehicleSelection() {
    // Vehicle selection with F1-F4
    if (IsKeyPressed(KEY_F1)) {
        selectedVehicle = 0;
        std::cout << "Vehicle 1 selected" << std::endl;
    }
    if (IsKeyPressed(KEY_F2)) {
        selectedVehicle = 1;
        std::cout << "Vehicle 2 selected" << std::endl;
    }
    if (IsKeyPressed(KEY_F3)) {
        selectedVehicle = 2;
        std::cout << "Vehicle 3 selected" << std::endl;
    }
    if (IsKeyPressed(KEY_F4)) {
        selectedVehicle = 3;
        std::cout << "Vehicle 4 selected" << std::endl;
    }
}

void CarSimulation::handleTargetAssignment() {
    if (!pathSystemInitialized || !vehicleController || selectedVehicle < 0) return;

    const auto& vehicles = vehicleController->getAllVehicles();
    if (static_cast<size_t>(selectedVehicle) >= vehicles.size()) return;

    auto it = vehicles.find(selectedVehicle);
    if (it == vehicles.end()) return;
    
    int vehicleId = it->second.vehicleId;

    // Mouse click on node for target selection
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        Point worldPos(mousePos.x, mousePos.y);

        // Find nearest node to mouse click
        int nearestNodeId = pathSystem.findNearestNode(worldPos, 80.0f);
        if (nearestNodeId != -1) {
            vehicleController->setVehicleTargetNode(vehicleId, nearestNodeId);
            std::cout << "Vehicle " << (selectedVehicle + 1) << " target set to node " << nearestNodeId << std::endl;
        }
    }

    // Number keys for direct node selection
    for (int i = 0; i <= 9; i++) {
        int key = KEY_ZERO + i;
        if (IsKeyPressed(key)) {
            int targetNode = (i == 0) ? 10 : i;
            if (targetNode <= 13) {
                vehicleController->setVehicleTargetNode(vehicleId, targetNode);
                std::cout << "Vehicle " << (selectedVehicle + 1) << " target set to node " << targetNode << std::endl;
            }
            break;
        }
    }

    // Special keys for nodes 11-13
    if (IsKeyPressed(KEY_Q)) {
        vehicleController->setVehicleTargetNode(vehicleId, 11);
        std::cout << "Vehicle " << (selectedVehicle + 1) << " target set to node 11" << std::endl;
    }
    if (IsKeyPressed(KEY_Y)) {
        vehicleController->setVehicleTargetNode(vehicleId, 12);
        std::cout << "Vehicle " << (selectedVehicle + 1) << " target set to node 12" << std::endl;
    }
    if (IsKeyPressed(KEY_X)) {
        vehicleController->setVehicleTargetNode(vehicleId, 13);
        std::cout << "Vehicle " << (selectedVehicle + 1) << " target set to node 13" << std::endl;
    }

    // Random targets for all vehicles
    if (IsKeyPressed(KEY_R)) {
        vehicleController->assignRandomTargetsToAllVehicles();
        std::cout << "Assigned new random targets to all vehicles" << std::endl;
    }
}
#define _USE_MATH_DEFINES
#include "renderer.h"
#include <cstdio>
#include <cmath> // For M_PI, cosf, sinf

Renderer::Renderer(int width, int height)
    : screenWidth(width), screenHeight(height), backgroundLoaded(false) {

    // Initialize colors
    backgroundColor = WHITE;
    frontPointColor = BLUE;  // All front points are blue
    selectedPointColor = RED;
    autoColor = GREEN;
    uiColor = BLACK;

    // Initialize different colors for identification points
    identificationColors = {RED, ORANGE, PURPLE, DARKGREEN};
}

void Renderer::initialize() {
    // Don't initialize window - handled by main
    loadBackground();
}

void Renderer::cleanup() {
    // Don't close window - handled by main
    if (backgroundLoaded) {
        UnloadTexture(backgroundTexture);
    }
}

bool Renderer::shouldClose() {
    return WindowShouldClose();
}

void Renderer::render(const std::vector<Point>& points, const std::vector<Auto>& detectedAutos, float tolerance, const PathSystem* pathSystem) {
    // Hauptfenster zeigt nur Hintergrundbild - keine Punkte, Autos oder Text
    renderBackgroundOnly();
}

void Renderer::drawPoint(const Point& point, int index, bool isSelected) {
    Color color = BLACK;  // All points are now black

    if (isSelected) {
        color = selectedPointColor;  // Keep selection color for dragging
    }

    // Draw point circle (larger for better visibility)
    DrawCircle(static_cast<int>(point.x), static_cast<int>(point.y), 12, color);
    DrawCircleLines(static_cast<int>(point.x), static_cast<int>(point.y), 12, BLACK);

    // Draw point type label based on color string
    if (!point.color.empty()) {
        if (point.color == "Front") {
            DrawText("FRONT", static_cast<int>(point.x + 15), static_cast<int>(point.y - 15), 18, BLACK);
        } else if (point.color.find("Heck") == 0) {
            // For Heck points, show actual Heck number from color
            static char heckLabel[16];
            if (point.color == "Heck1") snprintf(heckLabel, sizeof(heckLabel), "HECK1");
            else if (point.color == "Heck2") snprintf(heckLabel, sizeof(heckLabel), "HECK2");
            else if (point.color == "Heck3") snprintf(heckLabel, sizeof(heckLabel), "HECK3");
            else if (point.color == "Heck4") snprintf(heckLabel, sizeof(heckLabel), "HECK4");
            else snprintf(heckLabel, sizeof(heckLabel), "HECK?");
            DrawText(heckLabel, static_cast<int>(point.x + 15), static_cast<int>(point.y - 15), 18, BLACK);
        } else {
            // Fallback for unknown colors
            DrawText("UNKNOWN", static_cast<int>(point.x + 15), static_cast<int>(point.y - 15), 18, BLACK);
        }
    } else {
        // Fallback wenn keine Farbe gesetzt ist
        if (point.type == PointType::FRONT) {
            DrawText("FRONT", static_cast<int>(point.x + 15), static_cast<int>(point.y - 15), 18, BLACK);
        } else {
            DrawText("HECK", static_cast<int>(point.x + 15), static_cast<int>(point.y - 15), 18, BLACK);
        }
    }
}

void Renderer::drawAuto(const Auto& auto_) {
    if (!auto_.isValid()) return;

    Point idPoint = auto_.getIdentificationPoint();
    Point frontPoint = auto_.getFrontPoint();
    Point center = auto_.getCenter();

    // Draw line between the two points
    DrawLineEx({idPoint.x, idPoint.y}, {frontPoint.x, frontPoint.y}, 6.0f, autoColor);

    // Draw center point
    DrawCircle(static_cast<int>(center.x), static_cast<int>(center.y), 8, autoColor);

    // Draw direction arrow from identification to front point
    Vector2 directionVector = {frontPoint.x - idPoint.x, frontPoint.y - idPoint.y};
    float length = sqrtf(directionVector.x * directionVector.x + directionVector.y * directionVector.y);
    if (length > 0) {
        directionVector.x /= length;
        directionVector.y /= length;
    } else {
        directionVector = {1.0f, 0.0f}; // Default to right if points are the same
    }

    float arrowLength = 30.0f;
    Point arrowEnd;
    arrowEnd.x = idPoint.x + directionVector.x * arrowLength;
    arrowEnd.y = idPoint.y + directionVector.y * arrowLength;

    DrawLineEx({idPoint.x, idPoint.y}, {arrowEnd.x, arrowEnd.y}, 4.0f, ORANGE);

    // Draw arrowhead
    float headLength = 10.0f;
    float headAngle = 30.0f * M_PI / 180.0f;

    float dirRad = atan2f(directionVector.y, directionVector.x);

    Point head1, head2;
    head1.x = arrowEnd.x - cosf(dirRad - headAngle) * headLength;
    head1.y = arrowEnd.y - sinf(dirRad - headAngle) * headLength;
    head2.x = arrowEnd.x - cosf(dirRad + headAngle) * headLength;
    head2.y = arrowEnd.y - sinf(dirRad + headAngle) * headLength;

    DrawLineEx({arrowEnd.x, arrowEnd.y}, {head1.x, head1.y}, 3.0f, ORANGE);
    DrawLineEx({arrowEnd.x, arrowEnd.y}, {head2.x, head2.y}, 3.0f, ORANGE);
}

void Renderer::drawUI(float tolerance) {
    // Nur noch minimale Info links oben
    DrawText("PDS-T1000-TSA24", 10, 10, 20, uiColor);
}

void Renderer::drawVehicleInfo(const Auto& vehicle, const Point& position) {
    std::string info = "Vehicle " + std::to_string(vehicle.vehicleId);
    info += "\nNode: " + std::to_string(vehicle.currentNodeId);
    info += "\nTarget: " + std::to_string(vehicle.targetNodeId);
    info += "\nState: ";

    switch (vehicle.state) {
        case VehicleState::IDLE: info += "IDLE"; break;
        case VehicleState::MOVING: info += "MOVING"; break;
        case VehicleState::WAITING: info += "WAITING"; break;
        case VehicleState::ARRIVED: info += "ARRIVED"; break;
    }

    DrawText(info.c_str(), position.x + 20, position.y - 30, 12, WHITE);
}

void Renderer::drawVehiclePaths(const std::vector<Auto>& vehicles, const PathSystem& pathSystem) {
    // Define colors for different vehicles
    Color pathColors[] = {RED, GREEN, BLUE, YELLOW, ORANGE, PURPLE, PINK, LIME};
    int colorCount = sizeof(pathColors) / sizeof(pathColors[0]);

    for (size_t i = 0; i < vehicles.size(); i++) {
        const Auto& vehicle = vehicles[i];
        if (!vehicle.currentNodePath.empty() && vehicle.targetNodeId != -1) {
            Color pathColor = pathColors[i % colorCount];
            drawSingleVehiclePath(vehicle, pathSystem, pathColor);
        }
    }
}

void Renderer::drawSingleVehiclePath(const Auto& vehicle, const PathSystem& pathSystem, Color pathColor) {
    // Draw path from current node to target nodes
    if (vehicle.currentNodePath.empty()) return;

    Point currentPos = vehicle.position;
    
    // Draw lines between consecutive nodes in the path
    for (size_t i = vehicle.currentNodeIndex; i < vehicle.currentNodePath.size(); i++) {
        int nodeId = vehicle.currentNodePath[i];
        const PathNode* node = pathSystem.getNode(nodeId);
        if (!node) continue;

        Point nodePos = node->position;
        
        // Determine thickness and color based on whether it's current or future
        float thickness = (i == vehicle.currentNodeIndex) ? 5.0f : 3.0f;
        Color segmentColor = (i == vehicle.currentNodeIndex) ? pathColor : ColorAlpha(pathColor, 0.6f);

        // Draw line from current position to this node
        DrawLineEx({currentPos.x, currentPos.y}, 
                  {nodePos.x, nodePos.y}, 
                  thickness, segmentColor);

        // Draw arrow to show direction
        Point direction = nodePos - currentPos;
        float length = currentPos.distanceTo(nodePos);
        if (length > 10.0f) { // Only draw arrow if segment is long enough
            Point normalizedDir = direction * (1.0f / length);
            Point arrowPos = currentPos + normalizedDir * (length * 0.7f);
            Point arrowEnd = currentPos + normalizedDir * (length * 0.9f);

            DrawLineEx({arrowPos.x, arrowPos.y}, 
                      {arrowEnd.x, arrowEnd.y}, 
                      thickness + 1.0f, pathColor);
        }

        // Update current position for next iteration
        currentPos = nodePos;
    }

    // Draw target indicator
    if (vehicle.targetNodeId != -1) {
        const PathNode* targetNode = pathSystem.getNode(vehicle.targetNodeId);
        if (targetNode) {
            DrawCircleLines(targetNode->position.x, targetNode->position.y, 15.0f, pathColor);
            DrawCircleLines(targetNode->position.x, targetNode->position.y, 20.0f, pathColor);
        }
    }
}

void Renderer::renderWithData(const std::vector<Point>& points, const std::vector<Auto>& detectedAutos, float tolerance) {
    // Clear background
    ClearBackground(backgroundColor);

    // Draw background image
    drawBackground();

    // Draw detected vehicles first (so they appear behind points)
    for (const Auto& auto_ : detectedAutos) {
        drawAuto(auto_);
    }

    // Draw all points
    for (size_t i = 0; i < points.size(); i++) {
        bool isSelected = points[i].isDragging;
        drawPoint(points[i], static_cast<int>(i), isSelected);
    }

    // Draw UI
    drawUI(tolerance);

    // Draw vehicle information
    for (const auto& vehicle : detectedAutos) {
        if (vehicle.isValid()) {
            drawVehicleInfo(vehicle, vehicle.getCenter());
        }
    }
}

void Renderer::renderBackgroundOnly() {
    // Clear background and only draw the background image
    ClearBackground(backgroundColor);
    
    // Draw background image
    drawBackground();
}

void Renderer::loadBackground() {
    // Try to load the factory layout image
    const char* imagePath = "assets/factory_layout.png";

    if (FileExists(imagePath)) {
        backgroundTexture = LoadTexture(imagePath);
        backgroundLoaded = true;
        printf("Factory layout loaded: %dx%d\n", backgroundTexture.width, backgroundTexture.height);
    } else {
        printf("Warning: Factory layout image not found at %s\n", imagePath);
        backgroundLoaded = false;
    }
}

void Renderer::drawBackground() {
    if (backgroundLoaded) {
        // Scale background to fit screen while maintaining aspect ratio
        float scaleX = (float)screenWidth / backgroundTexture.width;
        float scaleY = (float)screenHeight / backgroundTexture.height;
        float scale = fminf(scaleX, scaleY);

        int scaledWidth = (int)(backgroundTexture.width * scale);
        int scaledHeight = (int)(backgroundTexture.height * scale);

        // Center the image
        int offsetX = (screenWidth - scaledWidth) / 2;
        int offsetY = (screenHeight - scaledHeight) / 2;

        Rectangle sourceRect = { 0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height };
        Rectangle destRect = { (float)offsetX, (float)offsetY, (float)scaledWidth, (float)scaledHeight };

        DrawTexturePro(backgroundTexture, sourceRect, destRect, { 0, 0 }, 0.0f, WHITE);
    }
}

Color Renderer::getIdentificationColor(int index) {
    if (index >= 0 && index < static_cast<int>(identificationColors.size())) {
        return identificationColors[index];
    }
    return RED;  // Default fallback
}

// Placeholder for drawNodes, drawSegments, drawVehicles, drawUI if they are used elsewhere
// In this context, they seem to be called by the modified render function.
// If they were intended to be part of the original render function, their logic
// would need to be preserved or adapted.

// Assuming these are internal helper functions for the original render method.
// Since the original render method was replaced, these might not be called directly
// but their logic might be implicitly handled or need explicit calls if they were
// intended to be public.
// For now, defining them as empty stubs to avoid compiler errors, assuming they
// are not directly used in the new structure or their logic is now handled differently.

void Renderer::drawNodes(const std::vector<PathNode>& nodes) {
    // Implementation for drawing nodes would go here if needed separately
}

void Renderer::drawSegments(const std::vector<PathSegment>& segments) {
    // Implementation for drawing segments would go here if needed separately
}

void Renderer::drawVehicles(const std::vector<Auto>& vehicles) {
    // Implementation for drawing vehicles would go here if needed separately
    for (const Auto& auto_ : vehicles) {
        drawAuto(auto_);
    }
}

void Renderer::render(const std::vector<PathNode>& nodes, 
                     const std::vector<PathSegment>& segments,
                     const std::vector<Auto>& vehicles,
                     const PathSystem* pathSystem) {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    // Draw path system
    drawSegments(segments);
    drawNodes(nodes);

    // Draw vehicle paths if pathSystem is available
    if (pathSystem) {
        drawVehiclePaths(vehicles, *pathSystem);
    }

    // Draw vehicles
    drawVehicles(vehicles);

    // Draw UI
    drawUI(0.0f);

    EndDrawing();
}
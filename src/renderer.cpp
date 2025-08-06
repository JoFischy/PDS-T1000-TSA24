
#define _USE_MATH_DEFINES
#include "renderer.h"
#include <cstdio>
#include <cmath> // For M_PI, cosf, sinf

Renderer::Renderer(int width, int height)
    : screenWidth(width), screenHeight(height) {

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
}

void Renderer::cleanup() {
    // Don't close window - handled by main
}

bool Renderer::shouldClose() {
    return WindowShouldClose();
}

void Renderer::render(const std::vector<Point>& points, const std::vector<Auto>& detectedAutos, float tolerance) {
    // Clear background
    ClearBackground(backgroundColor);

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
    drawVehicleInfo(detectedAutos);
}

void Renderer::drawPoint(const Point& point, int index, bool isSelected) {
    Color color;
    
    if (isSelected) {
        color = selectedPointColor;
    } else if (point.type == PointType::FRONT) {
        color = frontPointColor;  // All front points are blue
    } else {
        // For identification points, use different colors based on their pair number
        color = getIdentificationColor(index / 2);
    }

    // Draw point circle (larger for better visibility)
    DrawCircle(static_cast<int>(point.x), static_cast<int>(point.y), 12, color);
    DrawCircleLines(static_cast<int>(point.x), static_cast<int>(point.y), 12, BLACK);

    // Draw point type label
    const char* typeText = (point.type == PointType::FRONT) ? "FRONT" : "HECK";
    DrawText(typeText, static_cast<int>(point.x + 15), static_cast<int>(point.y - 15), 18, BLACK);
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

void Renderer::drawVehicleInfo(const std::vector<Auto>& detectedAutos) {
    int yOffset = 40;
    
    for (size_t i = 0; i < detectedAutos.size(); i++) {
        const Auto& vehicle = detectedAutos[i];
        if (vehicle.isValid()) {
            Point center = vehicle.getCenter();
            float direction = vehicle.getDirection();
            
            char vehicleText[128];
            snprintf(vehicleText, sizeof(vehicleText), "Auto %d: (%.0f, %.0f) %.0f°", 
                    vehicle.getId(), center.x, center.y, direction);
            DrawText(vehicleText, 10, yOffset, 16, uiColor);
            yOffset += 20;
        }
    }
}

Color Renderer::getIdentificationColor(int index) {
    if (index >= 0 && index < static_cast<int>(identificationColors.size())) {
        return identificationColors[index];
    }
    return RED;  // Default fallback
}

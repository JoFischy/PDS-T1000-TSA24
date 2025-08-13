
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

void Renderer::render(const std::vector<Point>& points, const std::vector<Auto>& detectedAutos, float tolerance) {
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

void Renderer::renderBackgroundOnly() {
    // Clear background
    ClearBackground(backgroundColor);
    
    // Draw only background image
    drawBackground();
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
    drawVehicleInfo(detectedAutos);
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

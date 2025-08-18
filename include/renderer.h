#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"
#include "point.h"
#include "auto.h"
#include "path_system.h" // Include path_system.h
#include <vector>
#include <string>
#include <sstream>

class Renderer {
private:
    int screenWidth;
    int screenHeight;

    // Background
    Texture2D backgroundTexture;
    bool backgroundLoaded;

    // Colors
    Color backgroundColor;
    Color frontPointColor;  // Blue for all front points
    Color selectedPointColor;
    Color autoColor;
    Color uiColor;
    std::vector<Color> identificationColors;  // Different colors for each identification point

public:
    Renderer(int width, int height);

    void initialize();
    void cleanup();
    bool shouldClose();
    // Original render signature for context, will be updated below
    // void render(const std::vector<Point>& points, const std::vector<Auto>& detectedAutos, float tolerance);
    
    // Updated render signature to include PathSystem
    void render(const std::vector<Point>& points, 
               const std::vector<Auto>& detectedAutos, 
               float tolerance,
               const PathSystem* pathSystem = nullptr);

    void renderBackgroundOnly();
    void renderWithData(const std::vector<Point>& points, const std::vector<Auto>& detectedAutos, float tolerance);
    
    // PathSystem specific render method
    void render(const std::vector<PathNode>& nodes, 
               const std::vector<PathSegment>& segments,
               const std::vector<Auto>& vehicles,
               const PathSystem* pathSystem = nullptr);

    // Public path visualization methods
    void drawVehiclePaths(const std::vector<Auto>& vehicles, const PathSystem& pathSystem);
    void drawSingleVehiclePath(const Auto& vehicle, const PathSystem& pathSystem, Color pathColor);

private:
    void drawPoint(const Point& point, int index, bool isSelected);
    void drawAuto(const Auto& auto_);
    void drawUI(float tolerance);
    void drawToleranceVisualization(const std::vector<Point>& points, float tolerance);
    void drawVehicleInfo(const std::vector<Auto>& detectedAutos); // This is likely a stub or needs to be refactored if drawVehicleInfo takes a single Auto
    void drawBackground();
    void loadBackground();
    Color getIdentificationColor(int index);

    // New path visualization methods
    void drawNodes(const std::vector<PathNode>& nodes);
    void drawSegments(const std::vector<PathSegment>& segments);
    void drawVehicles(const std::vector<Auto>& vehicles);
    void drawVehicleInfo(const Auto& vehicle, const Point& position);

};

#endif
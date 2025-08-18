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
    void drawVehiclePaths(const std::vector<Auto>& vehicles, const PathSystem& pathSystem);
    void drawSingleVehiclePath(const Auto& vehicle, const PathSystem& pathSystem, Color pathColor);
    // The original drawVehicleInfo(const Auto& vehicle, const Point& position) seems to be missing from the provided snippet, assuming it exists elsewhere or is a typo.
    // Based on the changes, it seems like there's a need for a drawVehicleInfo that might take a single auto.
    // If the original `drawVehicleInfo(const std::vector<Auto>& detectedAutos)` is meant to iterate, then the changes might be introducing a new overload or a different purpose.
    // Given the context, I'll assume the original `drawVehicleInfo(const std::vector<Auto>& detectedAutos)` is for drawing general vehicle info in a list, and the new methods are for specific path rendering.
    // The provided snippet for changes includes `drawVehicleInfo(const Auto& vehicle, const Point& position);` twice and as part of a block that replaces another block.
    // It's unclear if the original `drawVehicleInfo(const std::vector<Auto>& detectedAutos)` is intended to be replaced or augmented.
    // However, adhering strictly to the provided changes:
    // The first block of changes adds path visualization methods, including `drawVehicleInfo(const Auto& vehicle, const Point& position);`
    // The second block of changes seems to be a duplicate of the first, also adding `drawVehicleInfo(const Auto& vehicle, const Point& position);`
    // I will ensure `drawVehicleInfo(const Auto& vehicle, const Point& position);` is declared, and assume the other `drawVehicleInfo` remains or is handled elsewhere.
    // The original `drawVehicleInfo(const std::vector<Auto>& detectedAutos);` is kept as is, and the new method is added.
    // If the intention was to replace the vector version with the single version, that's not explicitly stated and would be an assumption.
    // I will add the declaration for `drawVehicleInfo(const Auto& vehicle, const Point& position);` as per the changes.
    void drawVehicleInfo(const Auto& vehicle, const Point& position); // Added as per changes

};

#endif
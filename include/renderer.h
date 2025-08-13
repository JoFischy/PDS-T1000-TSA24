
#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"
#include "point.h"
#include "auto.h"
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
    void render(const std::vector<Point>& points, const std::vector<Auto>& detectedAutos, float tolerance);
    void renderBackgroundOnly();
    void renderWithData(const std::vector<Point>& points, const std::vector<Auto>& detectedAutos, float tolerance);

private:
    void drawPoint(const Point& point, int index, bool isSelected);
    void drawAuto(const Auto& auto_);
    void drawUI(float tolerance);
    void drawToleranceVisualization(const std::vector<Point>& points, float tolerance);
    void drawVehicleInfo(const std::vector<Auto>& detectedAutos);
    void drawBackground();
    void loadBackground();
    Color getIdentificationColor(int index);
};

#endif

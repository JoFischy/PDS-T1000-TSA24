
#ifndef POINT_H
#define POINT_H

#include <string>

enum class PointType {
    IDENTIFICATION,
    FRONT
};

struct Point {
    float x;
    float y;
    bool isDragging;
    PointType type;
    std::string color; // Farbe des erkannten Objekts
    
    Point() : x(0.0f), y(0.0f), isDragging(false), type(PointType::IDENTIFICATION), color("") {}
    Point(float x, float y, PointType t = PointType::IDENTIFICATION, const std::string& c = "") : x(x), y(y), isDragging(false), type(t), color(c) {}
    
    // Calculate distance to another point
    float distanceTo(const Point& other) const;
    
    // Check if mouse is over this point
    bool isMouseOver(float mouseX, float mouseY, float radius = 10.0f) const;
};

#endif

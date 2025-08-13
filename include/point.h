
#ifndef POINT_H
#define POINT_H

#include <string>
#include <cmath>

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
    
    // Operators for point arithmetic
    Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y, type, color);
    }
    
    Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y, type, color);
    }
    
    Point operator*(float scalar) const {
        return Point(x * scalar, y * scalar, type, color);
    }
    
    Point normalize() const {
        float len = sqrt(x * x + y * y);
        if (len > 0.0f) {
            return Point(x / len, y / len, type, color);
        }
        return Point(0.0f, 0.0f, type, color);
    }
};

#endif

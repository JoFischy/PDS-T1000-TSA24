
#define _USE_MATH_DEFINES
#include "auto.h"
#include <cmath>

int Auto::nextId = 1;

Auto::Auto() : direction(0.0f), valid(false), id(0) {}

Auto::Auto(const Point& idPoint, const Point& fPoint) 
    : identificationPoint(idPoint), frontPoint(fPoint), valid(true), id(nextId++) {
    calculateCenterAndDirection();
}

void Auto::updatePoints(const Point& idPoint, const Point& fPoint) {
    identificationPoint = idPoint;
    frontPoint = fPoint;
    valid = true;
    calculateCenterAndDirection();
}

void Auto::calculateCenterAndDirection() {
    // Calculate center point
    center.x = (identificationPoint.x + frontPoint.x) / 2.0f;
    center.y = (identificationPoint.y + frontPoint.y) / 2.0f;
    
    // Calculate direction (vector from identification point to front point)
    // Adjust so 0° is up, 90° is right, 180° is down, 270° is left
    float dx = frontPoint.x - identificationPoint.x;
    float dy = frontPoint.y - identificationPoint.y;
    direction = atan2f(dx, -dy) * 180.0f / M_PI;  // Note: -dy to flip Y axis
    if (direction < 0) {
        direction += 360.0f;
    }
}


#define _USE_MATH_DEFINES
#include "auto.h"
#include <cmath>
#include <string>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int Auto::nextId = 1;

// Original detection-based constructors
Auto::Auto() : direction(0.0f), valid(false), id(0), vehicleId(0), currentNodeId(-1), targetNodeId(-1), 
               pendingTargetNodeId(-1), currentNodeIndex(0), state(VehicleState::IDLE), 
               currentDirection(Direction::NORTH), speed(50.0f), isMoving(false), isWaitingInQueue(false),
               currentSegmentId(-1) {}

Auto::Auto(const Point& idPoint, const Point& fPoint) 
    : identificationPoint(idPoint), frontPoint(fPoint), valid(true), vehicleId(0), currentNodeId(-1), 
      targetNodeId(-1), pendingTargetNodeId(-1), currentNodeIndex(0), state(VehicleState::IDLE),
      currentDirection(Direction::NORTH), speed(50.0f), isMoving(false), isWaitingInQueue(false),
      currentSegmentId(-1) {
    id = extractIdFromColor(idPoint.color);
    calculateCenterAndDirection();
}

// New path-system constructors
Auto::Auto(int id, const Point& startPos) 
    : identificationPoint(startPos), frontPoint(startPos), center(startPos), direction(0.0f), valid(true), id(id),
      vehicleId(id), position(startPos), targetPosition(startPos), currentNodeId(-1), targetNodeId(-1), pendingTargetNodeId(-1),
      currentNodeIndex(0), state(VehicleState::IDLE), currentDirection(Direction::NORTH), speed(50.0f),
      isMoving(false), isWaitingInQueue(false), currentSegmentId(-1) {
}

Auto::Auto(const Point& startPos, Direction dir) 
    : identificationPoint(startPos), frontPoint(startPos), center(startPos), direction(0.0f), valid(true), id(nextId++),
      vehicleId(id), position(startPos), targetPosition(startPos), currentNodeId(-1), targetNodeId(-1), pendingTargetNodeId(-1),
      currentNodeIndex(0), state(VehicleState::IDLE), currentDirection(dir), speed(50.0f),
      isMoving(false), isWaitingInQueue(false), currentSegmentId(-1) {
    center = startPos;
    direction = static_cast<float>(dir);
}

void Auto::updatePoints(const Point& idPoint, const Point& fPoint) {
    identificationPoint = idPoint;
    frontPoint = fPoint;
    valid = true;
    id = extractIdFromColor(idPoint.color);
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

int Auto::extractIdFromColor(const std::string& color) {
    // Extract number from color string like "Heck1", "Heck2", etc.
    if (color.find("Heck") == 0 && color.length() > 4) {
        try {
            std::string numberPart = color.substr(4); // Extract everything after "Heck"
            int extractedId = std::stoi(numberPart); // Extract number after "Heck"
            std::cout << "DEBUG: extractIdFromColor('" << color << "') -> ID=" << extractedId << std::endl;
            return extractedId;
        } catch (...) {
            std::cout << "DEBUG: extractIdFromColor('" << color << "') -> PARSE FAILED, defaulting to 0" << std::endl;
            return 0; // Default if parsing fails
        }
    }
    std::cout << "DEBUG: extractIdFromColor('" << color << "') -> NOT HECK COLOR, defaulting to 0" << std::endl;
    return 0; // Default for unknown colors
}

void Auto::setPosition(const Point& pos) {
    position = pos;
    center = pos; // Update center as well for compatibility
}

void Auto::setTargetPosition(const Point& target) {
    targetPosition = target;
    if (position.distanceTo(target) > 0.1f) {
        calculateDirection();
    }
}

void Auto::updatePosition(float deltaTime) {
    if (!isMoving || position.distanceTo(targetPosition) < 1.0f) {
        return;
    }

    // Move towards target position
    Point direction = targetPosition - position;
    float distance = position.distanceTo(targetPosition);

    if (distance > 0) {
        Point normalizedDir = direction * (1.0f / distance);
        float moveDistance = speed * deltaTime * 60.0f; // Assuming 60 FPS

        if (moveDistance >= distance) {
            position = targetPosition;
        } else {
            position = position + normalizedDir * moveDistance;
        }
    }
    
    // Update center for compatibility
    center = position;
}

void Auto::calculateDirection() {
    if (position.distanceTo(targetPosition) > 0.1f) {
        Point diff = targetPosition - position;
        direction = atan2(diff.y, diff.x) * 180.0f / M_PI;
        if (direction < 0) direction += 360.0f;
    }
}

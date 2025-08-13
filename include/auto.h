
#ifndef AUTO_H
#define AUTO_H

#include "point.h"
#include <vector>
#include <string>

enum class VehicleState {
    IDLE,
    MOVING,
    WAITING,
    ARRIVED
};

enum class Direction {
    NORTH = 0,
    EAST = 90,
    SOUTH = 180,
    WEST = 270,
    UNKNOWN = -1
};

class Auto {
private:
    Point identificationPoint;
    Point frontPoint;
    Point center;
    float direction;
    bool valid;
    int id;
    static int nextId;

    void calculateCenterAndDirection();
    int extractIdFromColor(const std::string& color);

public:
    // Original detection-based constructor
    Auto();
    Auto(const Point& idPoint, const Point& fPoint);

    // New path-system constructor
    Auto(int id, const Point& startPos);
    Auto(const Point& startPos, Direction dir);

    // Update auto with two points (original method)
    void updatePoints(const Point& idPoint, const Point& fPoint);

    // Vehicle properties for path system
    int vehicleId;
    Point position;
    Point targetPosition;
    int currentNodeId;
    int targetNodeId;
    int pendingTargetNodeId;
    
    // Path following
    std::vector<int> currentPath;
    size_t currentSegmentIndex;
    
    // State management
    VehicleState state;
    Direction currentDirection;
    float speed;
    
    // Movement flags
    bool isMoving;
    bool isWaitingInQueue;
    int currentSegmentId;

    // Position and movement methods
    void setPosition(const Point& pos);
    void setTargetPosition(const Point& target);
    void updatePosition(float deltaTime);
    void calculateDirection();

    // Getters (original)
    Point getCenter() const { return center; }
    Point getIdentificationPoint() const { return identificationPoint; }
    Point getFrontPoint() const { return frontPoint; }
    float getDirection() const { return direction; }
    bool isValid() const { return valid; }
    int getId() const { return id; }
};

#endif


#ifndef AUTO_H
#define AUTO_H

#include "point.h"

class Auto {
private:
    Point identificationPoint;
    Point frontPoint;
    Point center;
    float direction;
    bool valid;
    static int nextId;
    int id;

public:
    Auto();
    Auto(const Point& idPoint, const Point& fPoint);

    // Update auto with two points
    void updatePoints(const Point& idPoint, const Point& fPoint);

    // Getters
    Point getCenter() const { return center; }
    Point getIdentificationPoint() const { return identificationPoint; }
    Point getFrontPoint() const { return frontPoint; }
    float getDirection() const { return direction; }
    bool isValid() const { return valid; }
    int getId() const { return id; }
    
    // Static method to reset ID counter
    static void resetIdCounter() { nextId = 1; }

private:
    void calculateCenterAndDirection();
};

#endif

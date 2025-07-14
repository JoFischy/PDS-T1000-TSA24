#pragma once
#include <string>
#include <vector>
#include "../include/py_runner.h"

class CameraDisplay {
public:
    CameraDisplay();
    void update();
    void draw() const;
private:
    std::vector<CameraCoordinate> coordinates;
};

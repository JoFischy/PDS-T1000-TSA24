#pragma once
#include <string>
#include <vector>
#include "../include/py_runner.h"

class RaylibPythonAdder {
public:
    RaylibPythonAdder();
    void update();
    void draw() const;
private:
    std::vector<CameraCoordinate> coordinates;
};

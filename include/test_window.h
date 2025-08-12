#ifndef TEST_WINDOW_H
#define TEST_WINDOW_H

#include "Vehicle.h"
#include <vector>

// Startet ein separates Test-Fenster mit Windows API
void createWindowsAPITestWindow();

// Update-Funktion für Live-Koordinaten
void updateTestWindowCoordinates(const std::vector<DetectedObject>& detected_objects);

#endif

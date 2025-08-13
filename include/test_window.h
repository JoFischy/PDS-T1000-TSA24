#ifndef TEST_WINDOW_H
#define TEST_WINDOW_H

#include "Vehicle.h"
#include <vector>

// Forward declarations
class PathSystem;
class VehicleController;

// Startet ein separates Test-Fenster mit Windows API
void createWindowsAPITestWindow();

// Update-Funktion für Live-Koordinaten
void updateTestWindowCoordinates(const std::vector<DetectedObject>& detected_objects);

// Update-Funktion für erkannte Autos
void updateTestWindowVehicles(const std::vector<Auto>& vehicles);

// Setze PathSystem und VehicleController Referenzen für Anzeige
void setTestWindowPathSystem(const PathSystem* pathSystem, const VehicleController* vehicleController);

// Kalibrierte Koordinaten-Transformation (öffentlich verfügbar)
void getCalibratedTransform(float crop_x, float crop_y, float crop_width, float crop_height, float& fullscreen_x, float& fullscreen_y);

#endif

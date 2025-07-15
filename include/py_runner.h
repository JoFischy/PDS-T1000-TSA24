#pragma once
#include <string>
#include <vector>
#include "Vehicle.h"

// Initialize Python interpreter (call once at startup)
void initialize_python();

// === Fahrzeugflotte-Funktionen ===
// Initialize vehicle fleet with 4 predefined vehicles (all yellow front, different rear colors)
bool initialize_vehicle_fleet();

// Get detection data for all 4 vehicles with intelligent pairing
std::vector<VehicleDetectionData> get_all_vehicle_detections();

// Show camera feed with all vehicle detections and intelligent pairing
void show_fleet_camera_feed();

// Cleanup vehicle fleet resources
void cleanup_vehicle_fleet();

// Handle OpenCV window events
void handle_opencv_events();

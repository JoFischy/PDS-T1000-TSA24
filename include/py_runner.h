#pragma once
#include <string>
#include <vector>
#include "Vehicle.h"

// === KOORDINATEN-ERKENNUNG ===
// Get detected objects with normalized coordinates (mit automatischer Initialisierung)
std::vector<DetectedObject> get_detected_coordinates();

// Cleanup coordinate detector resources
void cleanup_coordinate_detector();

// === MONITOR 3 FUNKTIONEN ===
// Aktiviere Monitor 3 Modus für alle CV2-Fenster
bool enable_python_monitor3_mode();

// Deaktiviere Monitor 3 Modus
bool disable_python_monitor3_mode();

// Setze manuelle Monitor 3 Position
bool set_python_monitor3_position(int offset_x, int offset_y);

// Konfiguriere Monitor-Position nachträglich (nach Python-Initialisierung)
void configure_monitor_position_delayed(int offset_x, int offset_y);

// === LEGACY: Fahrzeugflotte-Funktionen (optional) ===
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

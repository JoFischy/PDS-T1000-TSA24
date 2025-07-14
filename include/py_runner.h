#pragma once
#include <string>
#include <vector>

// Structure to hold camera coordinates (legacy)
struct CameraCoordinate {
    int x, y, w, h;
};

// Structure for car detection data
struct CarDetectionData {
    // Red object (Heck)
    int red_x = 0;
    int red_y = 0;
    bool has_red = false;
    
    // Yellow object (Spitze)
    int yellow_x = 0;
    int yellow_y = 0;
    bool has_yellow = false;
    
    // Car orientation and distance
    float car_angle = 0.0f;      // Richtung in Grad (0-360°)
    float distance = 0.0f;       // Abstand zwischen Heck und Spitze
    bool has_angle = false;
    bool has_distance = false;
};

// Initialize Python interpreter (call once at startup)
void initialize_python();

// === Neue Auto-Erkennungs-Funktionen ===
// Gets car detection data (red=heck, yellow=spitze, angle, distance)
CarDetectionData get_car_detection_data();

// Gets car detection data with live camera display
CarDetectionData get_car_detection_with_display();

// === Legacy-Funktionen für Kompatibilität ===
// Gets red object coordinates from camera
std::vector<CameraCoordinate> get_camera_coordinates();

// Gets red object coordinates from camera and displays the video feed
std::vector<CameraCoordinate> get_camera_coordinates_with_display();

// Handle OpenCV window events
void handle_opencv_events();

// Cleanup camera resources
void cleanup_camera();

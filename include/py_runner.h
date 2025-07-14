#pragma once
#include <string>
#include <vector>

// Structure to hold camera coordinates
struct CameraCoordinate {
    int x, y, w, h;
};

// Initialize Python interpreter (call once at startup)
void initialize_python();

// Führt eine Python-Funktion aus und gibt das Ergebnis als int zurück
int run_python_add(int a, int b);

// Gets red object coordinates from camera
std::vector<CameraCoordinate> get_camera_coordinates();

// Gets red object coordinates from camera and displays the video feed
std::vector<CameraCoordinate> get_camera_coordinates_with_display();

// Handle OpenCV window events
void handle_opencv_events();

// Cleanup camera resources
void cleanup_camera();

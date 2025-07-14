
#include "raylib.h"
#include "../include/CameraDisplay.h"
#include "../include/py_runner.h"
#include <iostream>

int main() {
    try {
        // Initialize Python interpreter once at startup
        initialize_python();
        
        InitWindow(800, 600, "Raylib Live Camera Red Object Detection");
        CameraDisplay camera_detector;
        SetTargetFPS(30);  // 30 FPS for smooth camera updates
        
        while (!WindowShouldClose()) {
            // Update camera coordinates and display camera feed
            camera_detector.update();
            
            // Handle OpenCV window events
            handle_opencv_events();
            
            BeginDrawing();
            ClearBackground(RAYWHITE);
            camera_detector.draw();
            EndDrawing();
        }
        
        // Cleanup before closing
        cleanup_camera();
        CloseWindow();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

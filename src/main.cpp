
#include "raylib.h"
#include "../include/RaylibPythonAdder.h"
#include "../include/py_runner.h"

int main() {
    // Initialize Python interpreter once at startup
    initialize_python();
    
    InitWindow(800, 600, "Raylib Live Camera Red Object Detection");
    RaylibPythonAdder camera_detector;
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
    return 0;
}

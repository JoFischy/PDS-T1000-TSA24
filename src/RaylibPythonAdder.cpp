#include "../include/RaylibPythonAdder.h"
#include "raylib.h"
#include "../include/py_runner.h"
#include <string>
#include <chrono>
#include <ctime>

RaylibPythonAdder::RaylibPythonAdder() {
    // Initialize empty coordinates
}

void RaylibPythonAdder::update() {
    // Get new coordinates from camera with display
    coordinates = get_camera_coordinates_with_display();
}

void RaylibPythonAdder::draw() const {
    // Get current time for live indicator
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::tm* tm = std::localtime(&time_t);
    char time_str[50];
    sprintf(time_str, "LIVE: %02d:%02d:%02d.%03d", tm->tm_hour, tm->tm_min, tm->tm_sec, (int)ms.count());
    
    // Draw title with live indicator
    DrawText("Live Camera Red Object Detection", 10, 10, 24, DARKBLUE);
    DrawText(time_str, 450, 10, 16, GREEN);  // Live time indicator
    DrawText("Camera feed shown in separate window", 10, 40, 16, GRAY);
    
    if (coordinates.empty()) {
        DrawText("No red objects detected - showing test data", 10, 70, 16, RED);
    } else {
        std::string count_text = "Found " + std::to_string(coordinates.size()) + " red objects (LIVE):";
        DrawText(count_text.c_str(), 10, 70, 18, DARKGREEN);
    }
    
    // Draw each coordinate with more detail
    for (size_t i = 0; i < coordinates.size(); ++i) {
        const auto& coord = coordinates[i];
        
        // Object info text
        std::string obj_text = "Object " + std::to_string(i + 1) + ":";
        DrawText(obj_text.c_str(), 10, 100 + (int)i * 80, 16, DARKBLUE);
        
        std::string pos_text = "  Position: (" + std::to_string(coord.x) + ", " + std::to_string(coord.y) + ")";
        DrawText(pos_text.c_str(), 10, 120 + (int)i * 80, 14, BLUE);
        
        std::string size_text = "  Size: " + std::to_string(coord.w) + " x " + std::to_string(coord.h) + " pixels";
        DrawText(size_text.c_str(), 10, 135 + (int)i * 80, 14, BLUE);
        
        std::string area_text = "  Area: " + std::to_string(coord.w * coord.h) + " pixels";
        DrawText(area_text.c_str(), 10, 150 + (int)i * 80, 14, BLUE);
        
        // Draw a scaled preview rectangle
        int preview_x = 400;
        int preview_y = 100 + (int)i * 80;
        int scaled_w = coord.w / 3;  // Scale down for preview
        int scaled_h = coord.h / 3;
        
        // Ensure minimum size for visibility
        if (scaled_w < 10) scaled_w = 10;
        if (scaled_h < 10) scaled_h = 10;
        
        DrawText("Preview:", preview_x, preview_y, 14, DARKGRAY);
        DrawRectangleLines(preview_x, preview_y + 20, scaled_w, scaled_h, RED);
        DrawRectangle(preview_x, preview_y + 20, scaled_w, scaled_h, Color{255, 0, 0, 50}); // Semi-transparent fill
    }
    
    // Instructions with animation
    int blink = ((int)(GetTime() * 2)) % 2;  // Blink every 0.5 seconds
    Color instruction_color = blink ? DARKGRAY : GRAY;
    
    DrawText("Press ESC or close window to exit", 10, 550, 14, instruction_color);
    DrawText("Red objects are highlighted in camera window", 10, 570, 14, DARKGRAY);
}

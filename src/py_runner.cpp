#include "../include/py_runner.h"
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <iostream>
namespace py = pybind11;

// Global interpreter instance
static py::scoped_interpreter* guard = nullptr;
static py::object kamera_module;
static bool module_loaded = false;

void initialize_python() {
    if (guard == nullptr) {
        try {
            guard = new py::scoped_interpreter{};
            
            // Add current project's src directory to Python path
            py::module_ sys = py::module_::import("sys");
            py::object path = sys.attr("path");
            
            // Use absolute path to src directory for reliable loading
            std::string src_path = "C:/Users/jonas/OneDrive/Documents/GitHub/PDS-T1000-TSA24/src";
            path.attr("insert")(0, src_path);
            
            kamera_module = py::module_::import("Kamera");
            module_loaded = true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error initializing Python: " << e.what() << std::endl;
            throw;
        }
    }
}

int run_python_add(int a, int b) {
    initialize_python();
    py::object result = kamera_module.attr("add")(a, b);
    return result.cast<int>();
}

std::vector<CameraCoordinate> get_camera_coordinates() {
    initialize_python();
    
    try {
        py::object positions = kamera_module.attr("get_red_object_coordinates")();
        
        std::vector<CameraCoordinate> coordinates;
        py::list position_list = positions.cast<py::list>();
        
        for (auto item : position_list) {
            py::tuple pos = item.cast<py::tuple>();
            if (pos.size() == 4) {
                CameraCoordinate coord;
                coord.x = pos[0].cast<int>();
                coord.y = pos[1].cast<int>();
                coord.w = pos[2].cast<int>();
                coord.h = pos[3].cast<int>();
                coordinates.push_back(coord);
            }
        }
        
        return coordinates;
    } catch (const std::exception&) {
        // Return empty vector if there's an error
        return std::vector<CameraCoordinate>();
    }
}

std::vector<CameraCoordinate> get_camera_coordinates_with_display() {
    initialize_python();
    
    try {
        py::object positions = kamera_module.attr("get_red_object_coordinates_with_display")();
        
        std::vector<CameraCoordinate> coordinates;
        py::list position_list = positions.cast<py::list>();
        
        for (auto item : position_list) {
            py::tuple pos = item.cast<py::tuple>();
            if (pos.size() == 4) {
                CameraCoordinate coord;
                coord.x = pos[0].cast<int>();
                coord.y = pos[1].cast<int>();
                coord.w = pos[2].cast<int>();
                coord.h = pos[3].cast<int>();
                coordinates.push_back(coord);
            }
        }
        
        return coordinates;
    } catch (const std::exception&) {
        // Return empty vector if there's an error
        return std::vector<CameraCoordinate>();
    }
}

// Handle OpenCV window events
void handle_opencv_events() {
    if (!module_loaded) return;
    
    try {
        py::object cv2_module = py::module_::import("cv2");
        cv2_module.attr("waitKey")(1);  // Process OpenCV events
    } catch (const std::exception&) {
        // Ignore errors
    }
}

// Cleanup camera resources
void cleanup_camera() {
    if (!module_loaded) return;
    
    try {
        kamera_module.attr("cleanup_camera")();
        
        py::object cv2_module = py::module_::import("cv2");
        cv2_module.attr("destroyAllWindows")();
    } catch (const std::exception&) {
        // Ignore errors
    }
    
    if (guard != nullptr) {
        delete guard;
        guard = nullptr;
        module_loaded = false;
    }
}

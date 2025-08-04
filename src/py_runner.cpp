#include "../include/py_runner.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/embed.h"
#include <iostream>

namespace py = pybind11;

// Global Python state
static py::scoped_interpreter* python_interpreter = nullptr;
static py::module coordinate_detector_module;
static py::object detector_instance;

void initialize_python() {
    try {
        if (!python_interpreter) {
            python_interpreter = new py::scoped_interpreter();
            std::cout << "Python interpreter initialized." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Python initialization error: " << e.what() << std::endl;
    }
}

bool initialize_coordinate_detector() {
    try {
        // Import the new Farberkennung module
        py::module sys = py::module::import("sys");
        sys.attr("path").attr("insert")(0, "./src");  // Add src directory to Python path
        
        coordinate_detector_module = py::module::import("Farberkennung");
        
        // Create detector instance
        py::object detector_class = coordinate_detector_module.attr("SimpleCoordinateDetector");
        detector_instance = detector_class();
        
        // Initialize camera in Python
        py::object init_method = detector_instance.attr("initialize_camera");
        bool camera_ok = init_method().cast<bool>();
        
        if (!camera_ok) {
            std::cerr << "Failed to initialize camera in Python" << std::endl;
            return false;
        }
        
        // Create trackbars for interactive control
        py::object trackbar_method = detector_instance.attr("create_trackbars");
        trackbar_method();
        
        std::cout << "Coordinate detector initialized successfully." << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize coordinate detector: " << e.what() << std::endl;
        return false;
    }
}

std::vector<DetectedObject> get_detected_coordinates() {
    std::vector<DetectedObject> results;
    
    try {
        if (!detector_instance.is_none()) {
            // Call Python method to process one frame and get detections with visualization
            py::object process_method = detector_instance.attr("process_frame_with_display");
            py::list detection_list = process_method().cast<py::list>();
            
            results.clear();
            
            for (auto item : detection_list) {
                py::dict detection = item.cast<py::dict>();
                DetectedObject obj;
                
                obj.id = detection["id"].cast<int>();
                obj.color = detection["classified_color"].cast<std::string>();
                
                py::tuple coords = detection["normalized_coords"].cast<py::tuple>();
                obj.coordinates.x = coords[0].cast<float>();
                obj.coordinates.y = coords[1].cast<float>();
                
                obj.area = detection["area"].cast<float>();
                obj.crop_width = detection["crop_width"].cast<float>();
                obj.crop_height = detection["crop_height"].cast<float>();
                
                results.push_back(obj);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting coordinates: " << e.what() << std::endl;
    }
    
    return results;
}

void cleanup_coordinate_detector() {
    try {
        if (!detector_instance.is_none()) {
            // Clean up detector
            py::object cleanup_method = detector_instance.attr("cleanup");
            cleanup_method();
        }
        
        std::cout << "Coordinate detector cleaned up." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Cleanup error: " << e.what() << std::endl;
    }
}

// Legacy functions (empty implementations)
bool initialize_vehicle_fleet() {
    std::cout << "Warning: initialize_vehicle_fleet() is deprecated, use initialize_coordinate_detector() instead." << std::endl;
    return false;
}

std::vector<VehicleDetectionData> get_all_vehicle_detections() {
    std::cout << "Warning: get_all_vehicle_detections() is deprecated, use get_detected_coordinates() instead." << std::endl;
    return std::vector<VehicleDetectionData>();
}

void show_fleet_camera_feed() {
    // This is handled by the Python OpenCV windows now
}

void cleanup_vehicle_fleet() {
    std::cout << "Warning: cleanup_vehicle_fleet() is deprecated, use cleanup_coordinate_detector() instead." << std::endl;
}

void handle_opencv_events() {
    // OpenCV events are handled in Python
}

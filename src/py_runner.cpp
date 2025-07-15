#include "../include/py_runner.h"
#include "../include/VehicleFleet.h"
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <iostream>
#include <memory>

namespace py = pybind11;

// Global interpreter instance
static py::scoped_interpreter* guard = nullptr;

// Global vehicle fleet instance
static std::unique_ptr<VehicleFleet> global_fleet = nullptr;

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
            
            std::cout << "Python interpreter initialized for Multi-Vehicle Fleet" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error initializing Python: " << e.what() << std::endl;
            throw;
        }
    }
}

// Handle OpenCV window events
void handle_opencv_events() {
    try {
        py::object cv2_module = py::module_::import("cv2");
        cv2_module.attr("waitKey")(1);  // Process OpenCV events
    } catch (const std::exception&) {
        // Ignore errors
    }
}

// Legacy cleanup function (not used)
void cleanup_camera() {
    try {
        py::object cv2_module = py::module_::import("cv2");
        cv2_module.attr("destroyAllWindows")();
    } catch (const std::exception&) {
        // Ignore errors
    }
}

// === Fahrzeugflotte-Funktionen ===
bool initialize_vehicle_fleet() {
    try {
        global_fleet = std::make_unique<VehicleFleet>();
        
        // Konfiguriere 4 Fahrzeuge (alle Gelb vorne, verschiedene hintere Farben)
        global_fleet->add_vehicle("Auto-1", "Gelb", "Rot");      // Gelb vorne, Rot hinten
        global_fleet->add_vehicle("Auto-2", "Gelb", "Blau");     // Gelb vorne, Blau hinten  
        global_fleet->add_vehicle("Auto-3", "Gelb", "Grün");     // Gelb vorne, Grün hinten
        global_fleet->add_vehicle("Auto-4", "Gelb", "Lila");     // Gelb vorne, Lila hinten
        
        // Initialisiere Python Multi-Vehicle Detection
        py::object multi_vehicle_module = py::module_::import("MultiVehicleKamera");
        py::object init_result = multi_vehicle_module.attr("initialize_multi_vehicle_detection")();
        
        if (!init_result.cast<bool>()) {
            std::cerr << "Fehler: Multi-Vehicle Python-Initialisierung fehlgeschlagen!" << std::endl;
            return false;
        }
        
        std::cout << "Fahrzeugflotte mit " << global_fleet->get_vehicle_count() << " Fahrzeugen initialisiert!" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Fehler bei Fahrzeugflotte-Initialisierung: " << e.what() << std::endl;
        return false;
    }
}

std::vector<VehicleDetectionData> get_all_vehicle_detections() {
    std::vector<VehicleDetectionData> results;
    
    if (!global_fleet) {
        std::cerr << "Fahrzeugflotte nicht initialisiert!" << std::endl;
        return results;
    }
    
    try {
        py::object multi_vehicle_module = py::module_::import("MultiVehicleKamera");
        py::object detections = multi_vehicle_module.attr("get_multi_vehicle_detections")();
        
        py::list detection_list = detections.cast<py::list>();
        
        for (auto item : detection_list) {
            py::dict detection = item.cast<py::dict>();
            
            VehicleDetectionData vehicle_data;
            
            // Position extrahieren (vereinfachte Struktur)
            py::dict position = detection["position"].cast<py::dict>();
            vehicle_data.position = Point2D(position["x"].cast<float>(), position["y"].cast<float>());
            
            // Status und Eigenschaften
            vehicle_data.detected = detection["detected"].cast<bool>();
            vehicle_data.angle = detection["angle"].cast<float>();
            vehicle_data.distance = detection["distance"].cast<float>();
            vehicle_data.rear_color = detection["rear_color"].cast<std::string>();
            
            results.push_back(vehicle_data);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fehler bei Multi-Vehicle Detection: " << e.what() << std::endl;
    }
    
    return results;
}

void show_fleet_camera_feed() {
    try {
        py::object multi_vehicle_module = py::module_::import("MultiVehicleKamera");
        multi_vehicle_module.attr("show_multi_vehicle_feed")();
    } catch (const std::exception& e) {
        std::cerr << "Fehler bei Fleet Camera Feed: " << e.what() << std::endl;
    }
}

void cleanup_vehicle_fleet() {
    try {
        if (global_fleet) {
            py::object multi_vehicle_module = py::module_::import("MultiVehicleKamera");
            multi_vehicle_module.attr("cleanup_multi_vehicle_detection")();
            global_fleet.reset();
            std::cout << "Fahrzeugflotte bereinigt" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Fehler bei Fleet Cleanup: " << e.what() << std::endl;
    }
}



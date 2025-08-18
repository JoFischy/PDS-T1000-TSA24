#include "py_runner.h"
#include <iostream>
#include <Python.h>

static bool python_initialized = false;
static bool detector_initialized = false;
static bool monitor_configured = false;

bool initializePython() {
    if (python_initialized) return true;
    
    try {
        Py_Initialize();
        if (!Py_IsInitialized()) {
            std::cerr << "Failed to initialize Python" << std::endl;
            return false;
        }
        
        // Add current directory to Python path
        PyRun_SimpleString("import sys; sys.path.append('src')");
        python_initialized = true;
        return true;
    } catch (...) {
        std::cerr << "Exception during Python initialization" << std::endl;
        return false;
    }
}

void cleanupPython() {
    if (python_initialized) {
        Py_Finalize();
        python_initialized = false;
        detector_initialized = false;
    }
}

bool initializeDetector() {
    if (detector_initialized) return true;
    
    if (!initializePython()) {
        return false;
    }
    
    try {
        // Import Farberkennung module
        PyObject* pModule = PyImport_ImportModule("Farberkennung");
        if (!pModule) {
            PyErr_Print();
            return false;
        }
        
        // Get the initialization function
        PyObject* pFunc = PyObject_GetAttrString(pModule, "initialize_detector");
        if (!pFunc || !PyCallable_Check(pFunc)) {
            PyErr_Print();
            Py_DECREF(pModule);
            return false;
        }
        
        // Call the initialization function
        PyObject* pResult = PyObject_CallObject(pFunc, nullptr);
        bool success = false;
        if (pResult) {
            success = PyObject_IsTrue(pResult);
            Py_DECREF(pResult);
        }
        
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        
        if (success) {
            detector_initialized = true;
            // Initialisiert
        } else {
            // Fehler
        }
        
        return success;
    } catch (...) {
        std::cerr << "Exception during detector initialization" << std::endl;
        return false;
    }
}

std::vector<DetectedObject> runPythonDetection() {
    std::vector<DetectedObject> objects;
    
    if (!initializePython()) {
        return objects;
    }
    
    try {
        // Import Farberkennung module
        PyObject* pModule = PyImport_ImportModule("Farberkennung");
        if (!pModule) {
            PyErr_Print();
            return objects;
        }
        
        // Get the detection function
        PyObject* pFunc = PyObject_GetAttrString(pModule, "detect_objects");
        if (!pFunc || !PyCallable_Check(pFunc)) {
            PyErr_Print();
            Py_DECREF(pModule);
            return objects;
        }
        
        // Call the detection function
        PyObject* pResult = PyObject_CallObject(pFunc, nullptr);
        if (!pResult) {
            PyErr_Print();
            Py_DECREF(pFunc);
            Py_DECREF(pModule);
            return objects;
        }
        
        // Parse results - expecting list of dictionaries
        if (PyList_Check(pResult)) {
            Py_ssize_t size = PyList_Size(pResult);
            
            for (Py_ssize_t i = 0; i < size; i++) {
                PyObject* item = PyList_GetItem(pResult, i);
                if (PyDict_Check(item)) {
                    DetectedObject obj;
                    
                    // Get ID
                    PyObject* id = PyDict_GetItemString(item, "id");
                    if (id) {
                        obj.id = (int)PyLong_AsLong(id);
                    }
                    
                    // Get normalized coordinates - Python returns tuple
                    PyObject* coords = PyDict_GetItemString(item, "normalized_coords");
                    if (coords && PyTuple_Check(coords) && PyTuple_Size(coords) == 2) {
                        obj.coordinates.x = (float)PyFloat_AsDouble(PyTuple_GetItem(coords, 0));
                        obj.coordinates.y = (float)PyFloat_AsDouble(PyTuple_GetItem(coords, 1));
                    }
                    
                    // Get color
                    PyObject* color = PyDict_GetItemString(item, "classified_color");
                    if (color && PyUnicode_Check(color)) {
                        obj.color = PyUnicode_AsUTF8(color);
                    }
                    
                    // Get area
                    PyObject* area = PyDict_GetItemString(item, "area");
                    if (area) {
                        obj.area = (float)PyFloat_AsDouble(area);
                    }
                    
                    // Get crop dimensions - now from the object itself
                    PyObject* crop_width = PyDict_GetItemString(item, "crop_width");
                    if (crop_width) {
                        obj.crop_width = (float)PyFloat_AsDouble(crop_width);
                    }
                    
                    PyObject* crop_height = PyDict_GetItemString(item, "crop_height");
                    if (crop_height) {
                        obj.crop_height = (float)PyFloat_AsDouble(crop_height);
                    }
                    
                    // obj.is_valid = true; // Removed - field doesn't exist in DetectedObject
                    objects.push_back(obj);
                }
            }
        }
        
        Py_DECREF(pResult);
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        
    } catch (...) {
        std::cerr << "Exception during Python detection" << std::endl;
    }
    
    return objects;
}

std::vector<DetectedObject> get_detected_coordinates() {
    // Initialisiere Detektor beim ersten Aufruf
    if (!detector_initialized) {
        if (!initializeDetector()) {
            return std::vector<DetectedObject>();
        }
    }
    
    return runPythonDetection();
}

// Neue Funktion um Monitor-Position nachträglich zu setzen
void configure_monitor_position_delayed(int offset_x, int offset_y) {
    if (!monitor_configured && detector_initialized) {
        if (set_python_monitor3_position(offset_x, offset_y)) {
            monitor_configured = true;
            std::cout << "Monitor-Position nachträglich erfolgreich konfiguriert: " << offset_x << ", " << offset_y << std::endl;
        }
    }
}

void cleanup_coordinate_detector() {
    try {
        // Import Farberkennung module
        PyObject* pModule = PyImport_ImportModule("Farberkennung");
        if (pModule) {
            // Get the cleanup function
            PyObject* pFunc = PyObject_GetAttrString(pModule, "cleanup_detector");
            if (pFunc && PyCallable_Check(pFunc)) {
                // Call the cleanup function
                PyObject* pResult = PyObject_CallObject(pFunc, nullptr);
                if (pResult) {
                    Py_DECREF(pResult);
                }
                Py_DECREF(pFunc);
            }
            Py_DECREF(pModule);
        }
    } catch (...) {
        std::cerr << "Exception during detector cleanup" << std::endl;
    }
    cleanupPython();
}

// Legacy fleet functions (placeholders)
bool initialize_vehicle_fleet() {
    return true;
}

std::vector<VehicleDetectionData> get_all_vehicle_detections() {
    return std::vector<VehicleDetectionData>();
}

void show_fleet_camera_feed() {
    // Placeholder
}

void cleanup_vehicle_fleet() {
    // Placeholder
}

void handle_opencv_events() {
    // Placeholder
}

// === MONITOR 3 FUNKTIONEN ===
bool enable_python_monitor3_mode() {
    if (!initializePython()) {
        return false;
    }
    
    try {
        PyObject* pModule = PyImport_ImportModule("Farberkennung");
        if (!pModule) {
            PyErr_Print();
            return false;
        }
        
        PyObject* pFunc = PyObject_GetAttrString(pModule, "enable_monitor3_mode");
        if (!pFunc || !PyCallable_Check(pFunc)) {
            PyErr_Print();
            Py_DECREF(pModule);
            return false;
        }
        
        PyObject* pResult = PyObject_CallObject(pFunc, nullptr);
        bool success = false;
        if (pResult) {
            success = PyObject_IsTrue(pResult);
            Py_DECREF(pResult);
        }
        
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return success;
    } catch (...) {
        std::cerr << "Exception during enable_monitor3_mode" << std::endl;
        return false;
    }
}

bool disable_python_monitor3_mode() {
    if (!initializePython()) {
        return false;
    }
    
    try {
        PyObject* pModule = PyImport_ImportModule("Farberkennung");
        if (!pModule) {
            PyErr_Print();
            return false;
        }
        
        PyObject* pFunc = PyObject_GetAttrString(pModule, "disable_monitor3_mode");
        if (!pFunc || !PyCallable_Check(pFunc)) {
            PyErr_Print();
            Py_DECREF(pModule);
            return false;
        }
        
        PyObject* pResult = PyObject_CallObject(pFunc, nullptr);
        bool success = false;
        if (pResult) {
            success = PyObject_IsTrue(pResult);
            Py_DECREF(pResult);
        }
        
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return success;
    } catch (...) {
        std::cerr << "Exception during disable_monitor3_mode" << std::endl;
        return false;
    }
}

bool set_python_monitor3_position(int offset_x, int offset_y) {
    if (!initializePython()) {
        return false;
    }
    
    try {
        PyObject* pModule = PyImport_ImportModule("Farberkennung");
        if (!pModule) {
            PyErr_Print();
            return false;
        }
        
        PyObject* pFunc = PyObject_GetAttrString(pModule, "set_monitor3_position");
        if (!pFunc || !PyCallable_Check(pFunc)) {
            PyErr_Print();
            Py_DECREF(pModule);
            return false;
        }
        
        // Create arguments tuple
        PyObject* pArgs = PyTuple_New(2);
        PyTuple_SetItem(pArgs, 0, PyLong_FromLong(offset_x));
        PyTuple_SetItem(pArgs, 1, PyLong_FromLong(offset_y));
        
        PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
        bool success = false;
        if (pResult) {
            success = PyObject_IsTrue(pResult);
            Py_DECREF(pResult);
        }
        
        Py_DECREF(pArgs);
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return success;
    } catch (...) {
        std::cerr << "Exception during set_monitor3_position" << std::endl;
        return false;
    }
}
#pragma once
#include "Vehicle.h"
#include <vector>
#include <string>

// Vereinfachte Fahrzeugflotte - Computer Vision wird in Python gemacht
class VehicleFleet {
private:
    std::vector<std::string> vehicle_names;
    std::vector<std::string> front_colors;
    std::vector<std::string> rear_colors;
    
public:
    VehicleFleet();
    ~VehicleFleet();
    
    // Fahrzeug-Management (nur Konfiguration)
    void add_vehicle(const std::string& name, 
                     const std::string& front_color,
                     const std::string& rear_color);
    
    // Getter
    size_t get_vehicle_count() const { return vehicle_names.size(); }
    std::string get_vehicle_name(size_t index) const;
    std::string get_front_color(size_t index) const;
    std::string get_rear_color(size_t index) const;
};

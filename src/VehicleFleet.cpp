#include "../include/VehicleFleet.h"
#include <iostream>

VehicleFleet::VehicleFleet() {
}

VehicleFleet::~VehicleFleet() {
}

void VehicleFleet::add_vehicle(const std::string& name, 
                               const std::string& front_color,
                               const std::string& rear_color) {
    vehicle_names.push_back(name);
    front_colors.push_back(front_color);
    rear_colors.push_back(rear_color);
    std::cout << "Fahrzeug konfiguriert: " << name << " (Vorne: " << front_color << ", Hinten: " << rear_color << ")" << std::endl;
}

std::string VehicleFleet::get_vehicle_name(size_t index) const {
    if (index < vehicle_names.size()) {
        return vehicle_names[index];
    }
    return "";
}

std::string VehicleFleet::get_front_color(size_t index) const {
    if (index < front_colors.size()) {
        return front_colors[index];
    }
    return "";
}

std::string VehicleFleet::get_rear_color(size_t index) const {
    if (index < rear_colors.size()) {
        return rear_colors[index];
    }
    return "";
}

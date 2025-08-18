#include "vehicle_controller.h"
#include <iostream>
#include <cmath>

VehicleController::VehicleController(PathSystem* pathSys, SegmentManager* segMgr) 
    : pathSystem(pathSys), segmentManager(segMgr), nextVehicleId(1) {}

int VehicleController::addVehicle(const Point& position) {
    Auto vehicle;
    vehicle.vehicleId = nextVehicleId++;
    vehicle.setPosition(position);
    vehicle.currentNodeId = -1;
    vehicle.targetNodeId = -1;
    vehicle.state = VehicleState::ARRIVED;
    vehicle.currentNodePath.clear();
    vehicle.currentNodeIndex = 0;

    vehicles[vehicle.vehicleId] = vehicle;
    
    std::cout << "Added vehicle " << vehicle.vehicleId << " at position (" << position.x << ", " << position.y << ")" << std::endl;
    return vehicle.vehicleId;
}

Auto* VehicleController::getVehicle(int vehicleId) {
    auto it = vehicles.find(vehicleId);
    return (it != vehicles.end()) ? &it->second : nullptr;
}

const Auto* VehicleController::getVehicle(int vehicleId) const {
    auto it = vehicles.find(vehicleId);
    return (it != vehicles.end()) ? &it->second : nullptr;
}

void VehicleController::updateVehicleFromRealCoordinates(int vehicleId, const Point& realPosition, float confidence) {
    Auto* vehicle = getVehicle(vehicleId);
    if (!vehicle) return;

    Point oldPosition = vehicle->realWorldCoordinates;
    vehicle->realWorldCoordinates = realPosition;

    // Find nearest node if vehicle doesn't have one
    if (vehicle->currentNodeId == -1) {
        int nearestNode = pathSystem->findNearestNode(realPosition, 300.0f);
        if (nearestNode == -1) {
            nearestNode = pathSystem->findNearestNode(realPosition, 500.0f);
        }
        if (nearestNode != -1) {
            vehicle->currentNodeId = nearestNode;
            std::cout << "Vehicle " << vehicleId << " assigned to nearest node " << nearestNode << std::endl;
        }
    }

    // NEUE LOGIK: Knoten-basierte Navigation
    if (!vehicle->currentNodePath.empty() && vehicle->currentNodeIndex < vehicle->currentNodePath.size()) {
        int currentTargetNodeId = vehicle->currentNodePath[vehicle->currentNodeIndex];
        const PathNode* currentTargetNode = pathSystem->getNode(currentTargetNodeId);
        
        if (currentTargetNode) {
            Point targetPos = currentTargetNode->position;
            float distanceToTarget = pathSystem->calculateDistance(realPosition, targetPos);
            
            // Prüfe ob Knoten erreicht wurde
            float reachTolerance = 40.0f; // Toleranz für Knotenerreichung
            
            if (distanceToTarget < reachTolerance) {
                // Knoten erreicht!
                vehicle->currentNodeId = currentTargetNodeId;
                vehicle->currentNodeIndex++; // Gehe zum nächsten Knoten
                
                std::cout << "Vehicle " << vehicleId << " reached node " << currentTargetNodeId 
                          << " (distance: " << distanceToTarget << ")" << std::endl;
                
                if (vehicle->currentNodeIndex >= vehicle->currentNodePath.size()) {
                    // Route vollständig abgefahren
                    vehicle->state = VehicleState::ARRIVED;
                    vehicle->currentNodePath.clear();
                    vehicle->currentNodeIndex = 0;
                    std::cout << "Vehicle " << vehicleId << " completed full route and arrived at final target " 
                              << vehicle->targetNodeId << std::endl;
                } else {
                    // Nächster Knoten in der Route
                    int nextNodeId = vehicle->currentNodePath[vehicle->currentNodeIndex];
                    std::cout << "Vehicle " << vehicleId << " now targeting next node " << nextNodeId 
                              << " (step " << vehicle->currentNodeIndex << " of " << vehicle->currentNodePath.size() << ")" << std::endl;
                }
            }
        }
    }
}

bool VehicleController::setVehicleTargetNode(int vehicleId, int targetNodeId) {
    Auto* vehicle = getVehicle(vehicleId);
    if (!vehicle) return false;

    const PathNode* targetNode = pathSystem->getNode(targetNodeId);
    if (!targetNode) {
        std::cout << "Vehicle " << vehicleId << " target node " << targetNodeId << " does not exist" << std::endl;
        return false;
    }

    vehicle->targetNodeId = targetNodeId;
    
    if (planPath(vehicleId, targetNodeId)) {
        std::cout << "Vehicle " << vehicleId << " target set to node " << targetNodeId << std::endl;
        return true;
    } else {
        vehicle->state = VehicleState::WAITING;
        std::cout << "Vehicle " << vehicleId << " cannot reach target node " << targetNodeId << std::endl;
        return false;
    }
}

bool VehicleController::planPath(int vehicleId, int targetNodeId) {
    Auto* vehicle = getVehicle(vehicleId);
    if (!vehicle || vehicle->currentNodeId == -1) return false;

    const PathNode* targetNode = pathSystem->getNode(targetNodeId);
    if (!targetNode) {
        std::cout << "Vehicle " << vehicleId << " cannot plan path to non-existent node " << targetNodeId << std::endl;
        return false;
    }

    // Already at target?
    if (vehicle->currentNodeId == targetNodeId) {
        vehicle->state = VehicleState::ARRIVED;
        vehicle->currentNodePath.clear();
        vehicle->currentNodeIndex = 0;
        return true;
    }

    // NEUE LOGIK: Finde kompletten Knotenpfad (nicht Segmente!)
    std::vector<int> segmentPath = segmentManager->findOptimalPath(vehicle->currentNodeId, targetNodeId, vehicleId);
    
    if (segmentPath.empty()) {
        vehicle->state = VehicleState::WAITING;
        std::cout << "Vehicle " << vehicleId << " no path found to target" << std::endl;
        return false;
    }

    // Konvertiere Segment-Pfad zu Knoten-Pfad
    std::vector<int> nodePath;
    
    // Füge Startknoten hinzu
    nodePath.push_back(vehicle->currentNodeId);
    
    // Füge alle Zwischenknoten und Endknoten hinzu
    for (int segmentId : segmentPath) {
        const PathSegment* segment = pathSystem->getSegment(segmentId);
        if (segment) {
            // Finde den "anderen" Knoten (nicht den letzten in nodePath)
            int lastNode = nodePath.back();
            int nextNode = (segment->startNodeId == lastNode) ? segment->endNodeId : segment->startNodeId;
            nodePath.push_back(nextNode);
        }
    }

    // Setze neue Route
    vehicle->currentNodePath = nodePath;
    vehicle->currentNodeIndex = 1; // Index 0 ist der aktuelle Knoten, 1 ist das erste Ziel
    vehicle->targetNodeId = targetNodeId;
    vehicle->state = VehicleState::IDLE;
    
    std::cout << "Vehicle " << vehicleId << " planned node path with " << nodePath.size() << " nodes: ";
    for (size_t i = 0; i < nodePath.size(); i++) {
        std::cout << nodePath[i];
        if (i < nodePath.size() - 1) std::cout << " -> ";
    }
    std::cout << std::endl;
    
    return true;
}

int VehicleController::mapRealVehicleToSystem(const Point& realPosition, const std::string& vehicleColor) {
    // Check if we already have a mapping for this color
    auto it = colorToVehicleId.find(vehicleColor);
    if (it != colorToVehicleId.end()) {
        return it->second;
    }

    // Create new vehicle for this color
    int vehicleId = addVehicle(realPosition);
    colorToVehicleId[vehicleColor] = vehicleId;

    std::cout << "Mapped new vehicle " << vehicleColor << " to ID " << vehicleId << std::endl;
    return vehicleId;
}

void VehicleController::syncRealVehiclesWithSystem(const std::vector<Auto>& detectedAutos) {
    for (const Auto& detectedVehicle : detectedAutos) {
        std::string vehicleColor = detectedVehicle.colorValue;
        Point realPosition = detectedVehicle.realWorldCoordinates;
        
        int systemVehicleId = mapRealVehicleToSystem(realPosition, vehicleColor);
        updateVehicleFromRealCoordinates(systemVehicleId, realPosition, 1.0f);
    }
}

void VehicleController::assignRandomTargetsToAllVehicles() {
    if (pathSystem->getNodeCount() == 0) return;

    for (auto& [vehicleId, vehicle] : vehicles) {
        if (vehicle.state == VehicleState::ARRIVED || vehicle.targetNodeId == -1) {
            // Find available target node (any node type is valid)
            std::vector<int> availableNodes;
            for (const auto& node : pathSystem->getNodes()) {
                if (node.nodeId != vehicle.currentNodeId) {
                    availableNodes.push_back(node.nodeId);
                }
            }
            
            if (!availableNodes.empty()) {
                int randomIndex = rand() % availableNodes.size();
                int targetNode = availableNodes[randomIndex];
                
                if (setVehicleTargetNode(vehicleId, targetNode)) {
                    std::cout << "Vehicle " << vehicleId << " assigned random target: node " << targetNode << std::endl;
                } else {
                    vehicle.state = VehicleState::ARRIVED;
                }
            }
        }
    }
}

void VehicleController::updateVehiclePaths() {
    // Simplified - paths are managed by planPath function
}

void VehicleController::coordinateVehicleMovements() {
    // Simplified - coordination happens in planPath
}

void VehicleController::updateVehicleMovement(Auto& vehicle, float deltaTime) {
    // Movement simulation - simplified
}

void VehicleController::moveVehicleAlongPath(Auto& vehicle, float deltaTime) {
    // Path following - simplified
}

void VehicleController::handleBlockedVehicle(Auto& vehicle) {
    // Blocking handling - simplified
}

std::string VehicleController::getVehicleStateString(VehicleState state) const {
    switch (state) {
        case VehicleState::IDLE: return "IDLE";
        case VehicleState::MOVING: return "MOVING";
        case VehicleState::WAITING: return "WAITING";
        case VehicleState::ARRIVED: return "ARRIVED";
        default: return "UNKNOWN";
    }
}

// getPathSystem is already defined inline in header

// getAllVehicles is already defined inline in header

bool VehicleController::hasVehicleArrived(int vehicleId) const {
    const Auto* vehicle = getVehicle(vehicleId);
    return vehicle && vehicle->state == VehicleState::ARRIVED;
}

std::vector<int> VehicleController::getActiveVehicleIds() const {
    std::vector<int> activeIds;
    for (const auto& [vehicleId, vehicle] : vehicles) {
        activeIds.push_back(vehicleId);
    }
    return activeIds;
}

bool VehicleController::replanPathIfBlocked(int vehicleId) { 
    return false; 
}

void VehicleController::updateVehicles(float deltaTime) {
    // Update all vehicles in the new node-based system
    for (auto& [vehicleId, vehicle] : vehicles) {
        updateVehicleMovement(vehicle, deltaTime);
    }
}

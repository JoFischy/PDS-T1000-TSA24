#include "vehicle_controller.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <thread>
#include <chrono>
#include <unordered_map>

VehicleController::VehicleController(PathSystem* pathSys, SegmentManager* segmentMgr)
    : pathSystem(pathSys), segmentManager(segmentMgr), nextVehicleId(0) {}

int VehicleController::addVehicle(const Point& startPosition) {
    Auto vehicle(nextVehicleId, startPosition);

    // Find nearest node as starting position
    int nearestNodeId = pathSystem->findNearestNode(startPosition);
    if (nearestNodeId != -1) {
        vehicle.currentNodeId = nearestNodeId;
        const PathNode* node = pathSystem->getNode(nearestNodeId);
        if (node) {
            vehicle.position = node->position;
        }
    }

    // Vehicle starts without target - will stay at spawn until target is set
    vehicle.targetNodeId = -1;
    vehicle.state = VehicleState::ARRIVED;

    vehicles.push_back(vehicle);
    vehicleIdToIndex[nextVehicleId] = vehicles.size() - 1;

    return nextVehicleId++;
}

void VehicleController::spawnInitialVehicles() {
    if (!pathSystem || pathSystem->getNodeCount() < 4) {
        std::cerr << "Not enough nodes to spawn 4 vehicles" << std::endl;
        return;
    }

    const auto& nodes = pathSystem->getNodes();

    // Clear existing vehicles
    vehicles.clear();
    vehicleIdToIndex.clear();
    colorToVehicleId.clear();
    nextVehicleId = 0;

    // Spawn 4 vehicles at different nodes
    int nodeStep = std::max(1, static_cast<int>(nodes.size()) / 4);

    for (int i = 0; i < 4 && static_cast<size_t>(i * nodeStep) < nodes.size(); i++) {
        const PathNode& node = nodes[i * nodeStep];
        int vehicleId = addVehicle(node.position);

        // Map colors to vehicle IDs
        std::string colorKey = "Heck" + std::to_string(i + 1);
        colorToVehicleId[colorKey] = vehicleId;

        std::cout << "Spawned vehicle " << (i + 1) << " at node " << node.nodeId
                  << " (" << node.position.x << ", " << node.position.y << ")" << std::endl;
    }

    std::cout << "Spawned " << vehicles.size() << " initial vehicles (no targets assigned)" << std::endl;
}

void VehicleController::updateVehicleFromRealCoordinates(int vehicleId, const Point& realPosition, float realDirection) {
    Auto* vehicle = getVehicle(vehicleId);
    if (!vehicle) return;

    // Store old position for change detection
    Point oldPosition = vehicle->position;
    int oldNodeId = vehicle->currentNodeId;

    // Update position from real coordinates
    vehicle->position = realPosition;
    vehicle->currentDirection = static_cast<Direction>((int)realDirection);

    // If vehicle doesn't have a current node or is far from it, update current node
    if (vehicle->currentNodeId == -1) {
        // Try increasing search radius to find a node
        int nearestNode = pathSystem->findNearestNode(realPosition, 200.0f);
        if (nearestNode == -1) {
            // If still no node found, try even larger radius
            nearestNode = pathSystem->findNearestNode(realPosition, 500.0f);
        }
        if (nearestNode != -1) {
            vehicle->currentNodeId = nearestNode;
            std::cout << "Vehicle " << vehicleId << " snapped to node " << nearestNode << " from off-path position" << std::endl;
        } else {
            std::cout << "Warning: Vehicle " << vehicleId << " cannot find any nearby node!" << std::endl;
        }
    } else {
        // Check if vehicle is still near its current node
        const PathNode* currentNode = pathSystem->getNode(vehicle->currentNodeId);
        if (currentNode) {
            float distanceToCurrentNode = realPosition.distanceTo(currentNode->position);
            if (distanceToCurrentNode > 200.0f) {
                // Vehicle moved far from current node, find new nearest node with extended search
                int nearestNode = pathSystem->findNearestNode(realPosition, 300.0f);
                if (nearestNode == -1) {
                    // Try even larger radius if needed
                    nearestNode = pathSystem->findNearestNode(realPosition, 500.0f);
                }
                if (nearestNode != -1 && nearestNode != vehicle->currentNodeId) {
                    vehicle->currentNodeId = nearestNode;
                    std::cout << "Vehicle " << vehicleId << " relocated to node " << nearestNode << " (was off-path)" << std::endl;

                    // If vehicle has a target and moved to different node, replan route
                    if (vehicle->targetNodeId != -1 && nearestNode != oldNodeId) {
                        if (planPath(vehicleId, vehicle->targetNodeId)) {
                            std::cout << "Vehicle " << vehicleId << " route replanned due to position change" << std::endl;
                        }
                    }
                }
            }
        }
    }

    // Check if vehicle was moved significantly and adjust route accordingly
    float movementDistance = oldPosition.distanceTo(realPosition);
    if (movementDistance > 80.0f && vehicle->targetNodeId != -1) {
        // Vehicle was moved manually - check if current path is still valid
        if (!vehicle->currentPath.empty() && vehicle->currentSegmentIndex < vehicle->currentPath.size()) {
            // Clear current path progress and replan
            vehicle->currentSegmentIndex = 0;
            vehicle->state = VehicleState::IDLE;
        }
    }
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
    for (const auto& detectedAuto : detectedAutos) {
        if (!detectedAuto.isValid()) continue;

        // Extract vehicle color/ID from detected auto
        std::string vehicleColor = "Heck" + std::to_string(detectedAuto.getId());

        // Map to system vehicle
        int vehicleId = mapRealVehicleToSystem(detectedAuto.getCenter(), vehicleColor);

        // Update position and direction
        updateVehicleFromRealCoordinates(vehicleId, detectedAuto.getCenter(), detectedAuto.getDirection());
    }
}

Auto* VehicleController::getVehicle(int vehicleId) {
    auto it = vehicleIdToIndex.find(vehicleId);
    return (it != vehicleIdToIndex.end()) ? &vehicles[it->second] : nullptr;
}

const Auto* VehicleController::getVehicle(int vehicleId) const {
    auto it = vehicleIdToIndex.find(vehicleId);
    return (it != vehicleIdToIndex.end()) ? &vehicles[it->second] : nullptr;
}

void VehicleController::setVehicleTargetNode(int vehicleId, int targetNodeId) {
    Auto* vehicle = getVehicle(vehicleId);
    if (!vehicle) return;

    // Check if target node is a waiting node - not allowed as target
    const PathNode* targetNode = pathSystem->getNode(targetNodeId);
    if (targetNode && targetNode->isWaitingNode) {
        std::cout << "Vehicle " << vehicleId << " cannot target waiting node " << targetNodeId << std::endl;
        return;
    }

    vehicle->targetNodeId = targetNodeId;
    vehicle->pendingTargetNodeId = -1;

    // Plan new route if target is different from current position
    if (vehicle->currentNodeId != -1 && vehicle->currentNodeId != targetNodeId) {
        if (planPath(vehicleId, targetNodeId)) {
            vehicle->state = VehicleState::IDLE;
        } else {
            vehicle->state = VehicleState::WAITING;
        }
    } else {
        // Already at target
        vehicle->state = VehicleState::ARRIVED;
    }

    std::cout << "Vehicle " << vehicleId << " new target set to node " << targetNodeId << std::endl;
}

void VehicleController::assignRandomTargetsToAllVehicles() {
    if (!pathSystem || pathSystem->getNodeCount() < 2) return;

    const auto& nodes = pathSystem->getNodes();

    // Filter out waiting nodes - only use regular nodes as targets
    std::vector<int> validTargetNodes;
    for (const auto& node : nodes) {
        if (!node.isWaitingNode) {
            validTargetNodes.push_back(node.nodeId);
        }
    }

    if (validTargetNodes.empty()) {
        std::cout << "No valid target nodes available (no non-waiting nodes)" << std::endl;
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> nodeDist(0, validTargetNodes.size() - 1);

    for (auto& vehicle : vehicles) {
        // Find a different target node than current one
        int targetNodeId;
        do {
            int randomIndex = nodeDist(gen);
            targetNodeId = validTargetNodes[randomIndex];
        } while (targetNodeId == vehicle.currentNodeId && validTargetNodes.size() > 1);

        setVehicleTargetNode(vehicle.vehicleId, targetNodeId);
        std::cout << "Vehicle " << vehicle.vehicleId << " assigned target node " << targetNodeId << std::endl;
    }
}

bool VehicleController::planPath(int vehicleId, int targetNodeId) {
    Auto* vehicle = getVehicle(vehicleId);
    if (!vehicle || vehicle->currentNodeId == -1) return false;

    // Check if target node is a waiting node - not allowed as target
    const PathNode* targetNode = pathSystem->getNode(targetNodeId);
    if (targetNode && targetNode->isWaitingNode) {
        std::cout << "Vehicle " << vehicleId << " cannot plan path to waiting node " << targetNodeId << std::endl;
        return false;
    }

    // Already at target?
    if (vehicle->currentNodeId == targetNodeId) {
        vehicle->state = VehicleState::ARRIVED;
        vehicle->currentPath.clear();
        return true;
    }

    // First try to find path avoiding currently occupied segments
    std::vector<int> path = segmentManager->findAvailablePath(vehicle->currentNodeId, targetNodeId, vehicleId);

    // If no available path found, use optimal path (vehicles will wait for segments)
    if (path.empty()) {
        path = segmentManager->findOptimalPath(vehicle->currentNodeId, targetNodeId, vehicleId);
    }

    if (!path.empty()) {
        vehicle->currentPath = path;
        vehicle->currentSegmentIndex = 0;
        vehicle->targetNodeId = targetNodeId;
        vehicle->state = VehicleState::IDLE;
        std::cout << "Vehicle " << vehicleId << " planned path with " << path.size() << " segments" << std::endl;
        return true;
    } else {
        vehicle->state = VehicleState::WAITING;
        std::cout << "Vehicle " << vehicleId << " no path found to target" << std::endl;
        return false;
    }
}

void VehicleController::updateVehiclePaths() {
    for (Auto& vehicle : vehicles) {
        // If vehicle has a target and is not already moving towards it or has an invalid path
        if (vehicle.targetNodeId != -1) {
            // If vehicle is not on a path, or has reached the end of its current path, or its current node is no longer the start of the next segment
            if (vehicle.currentPath.empty() ||
                vehicle.currentSegmentIndex >= vehicle.currentPath.size() ||
                (vehicle.currentSegmentIndex < vehicle.currentPath.size() &&
                 pathSystem->getSegment(vehicle.currentPath[vehicle.currentSegmentIndex])->startNodeId != vehicle.currentNodeId))
            {
                // If the vehicle is not at the target, replan
                if (vehicle.currentNodeId != vehicle.targetNodeId) {
                    if (planPath(vehicle.vehicleId, vehicle.targetNodeId)) {
                        std::cout << "Vehicle " << vehicle.vehicleId << " continuously replanned path to node " << vehicle.targetNodeId << std::endl;
                    }
                } else {
                    // Vehicle is at the target
                    vehicle.state = VehicleState::ARRIVED;
                    vehicle.currentPath.clear();
                }
            }
        }
    }
}

void VehicleController::updateVehicleMovements(float deltaTime) {
    for (Auto& vehicle : vehicles) {
        updateVehicleMovement(vehicle, deltaTime);
    }

    // Update segment manager queues
    segmentManager->updateQueues();
}

void VehicleController::updateVehicles(float deltaTime) {
    // This is the main update function called from the simulation
    updateVehicleMovements(deltaTime);
    coordinateVehicleMovements();
}

void VehicleController::coordinateVehicleMovements() {
    // Basic coordination - can be expanded
    for (Auto& vehicle : vehicles) {
        if (vehicle.state == VehicleState::WAITING && vehicle.targetNodeId != -1) {
            // Try to find alternative routes for waiting vehicles
            if (!vehicle.currentPath.empty()) {
                int nextSegmentId = vehicle.currentPath[vehicle.currentSegmentIndex];
                const PathSegment* segment = pathSystem->getSegment(nextSegmentId);
                if (segment && segment->isOccupied && segment->occupiedByVehicleId != vehicle.vehicleId) {
                    // Try to find alternative path
                    std::vector<int> altPath = segmentManager->findAvailablePath(
                        vehicle.currentNodeId, vehicle.targetNodeId, vehicle.vehicleId);

                    if (!altPath.empty() && altPath != vehicle.currentPath) {
                        vehicle.currentPath = altPath;
                        vehicle.currentSegmentIndex = 0;
                        vehicle.state = VehicleState::IDLE;
                        std::cout << "Vehicle " << vehicle.vehicleId << " found alternative path" << std::endl;
                    }
                }
            }
        }
    }
}

void VehicleController::updateVehicleMovement(Auto& vehicle, float deltaTime) {
    switch (vehicle.state) {
        case VehicleState::IDLE:
            // Try to proceed to next segment
            if (vehicle.targetNodeId != -1 && vehicle.currentNodeId != vehicle.targetNodeId) {
                if (!vehicle.currentPath.empty() && vehicle.currentSegmentIndex < vehicle.currentPath.size()) {
                    int nextSegmentId = vehicle.currentPath[vehicle.currentSegmentIndex];
                    if (segmentManager->canVehicleEnterSegment(nextSegmentId, vehicle.vehicleId)) {
                        if (segmentManager->reserveSegment(nextSegmentId, vehicle.vehicleId)) {
                            vehicle.state = VehicleState::MOVING;
                        } else {
                            vehicle.state = VehicleState::WAITING;
                            segmentManager->addToQueue(nextSegmentId, vehicle.vehicleId);
                        }
                    } else {
                        vehicle.state = VehicleState::WAITING;
                        segmentManager->addToQueue(nextSegmentId, vehicle.vehicleId);
                    }
                }
            }
            break;

        case VehicleState::MOVING:
            moveVehicleAlongPath(vehicle, deltaTime);
            break;

        case VehicleState::WAITING:
            handleBlockedVehicle(vehicle);
            break;

        case VehicleState::ARRIVED:
            // Vehicle stays at destination
            break;
    }
}

void VehicleController::moveVehicleAlongPath(Auto& vehicle, float deltaTime) {
    if (vehicle.currentPath.empty() || vehicle.currentSegmentIndex >= vehicle.currentPath.size()) {
        vehicle.state = VehicleState::ARRIVED;
        return;
    }

    int currentSegmentId = vehicle.currentPath[vehicle.currentSegmentIndex];
    const PathSegment* segment = pathSystem->getSegment(currentSegmentId);
    if (!segment) {
        vehicle.state = VehicleState::WAITING;
        return;
    }

    // Verify that vehicle has reserved this segment
    if (segment->occupiedByVehicleId != vehicle.vehicleId) {
        vehicle.state = VehicleState::WAITING;
        return;
    }

    const PathNode* startNode = pathSystem->getNode(segment->startNodeId);
    const PathNode* endNode = pathSystem->getNode(segment->endNodeId);
    if (!startNode || !endNode) {
        segmentManager->releaseSegment(currentSegmentId, vehicle.vehicleId);
        vehicle.state = VehicleState::WAITING;
        return;
    }

    // Determine target position
    Point targetPos;
    int targetNodeId;

    // Determine which end of the segment to go to
    float distToStart = vehicle.position.distanceTo(startNode->position);
    float distToEnd = vehicle.position.distanceTo(endNode->position);

    if (distToStart > distToEnd) {
        targetPos = endNode->position;
        targetNodeId = endNode->nodeId;
    } else {
        targetPos = startNode->position;
        targetNodeId = startNode->nodeId;
    }

    // Calculate movement
    float moveDistance = vehicle.speed * deltaTime;
    Point direction = (targetPos - vehicle.position).normalize();
    float distanceToTarget = vehicle.position.distanceTo(targetPos);

    if (moveDistance >= distanceToTarget || distanceToTarget < 5.0f) {
        // Arrived at segment target node
        vehicle.position = targetPos;
        vehicle.currentNodeId = targetNodeId;

        // Release segment
        segmentManager->releaseSegment(currentSegmentId, vehicle.vehicleId);

        // Move to next segment
        vehicle.currentSegmentIndex++;

        if (vehicle.currentSegmentIndex >= vehicle.currentPath.size()) {
            // Reached final target
            vehicle.state = VehicleState::ARRIVED;
            vehicle.currentPath.clear();
            std::cout << "Vehicle " << vehicle.vehicleId << " arrived at target node " << vehicle.targetNodeId << std::endl;
        } else {
            // Back to IDLE for next segment
            vehicle.state = VehicleState::IDLE;
        }
    } else {
        // Continue moving towards target node
        vehicle.position = vehicle.position + direction * moveDistance;
    }
}

void VehicleController::handleBlockedVehicle(Auto& vehicle) {
    if (vehicle.state != VehicleState::WAITING) return;

    // Try to reserve next segment
    if (!vehicle.currentPath.empty() && vehicle.currentSegmentIndex < vehicle.currentPath.size()) {
        int nextSegmentId = vehicle.currentPath[vehicle.currentSegmentIndex];

        if (segmentManager->canVehicleEnterSegment(nextSegmentId, vehicle.vehicleId)) {
            if (segmentManager->reserveSegment(nextSegmentId, vehicle.vehicleId)) {
                vehicle.state = VehicleState::MOVING;
                std::cout << "Vehicle " << vehicle.vehicleId << " successfully reserved segment " << nextSegmentId << std::endl;
                return;
            }
        }

        // Add to queue if not already there
        segmentManager->addToQueue(nextSegmentId, vehicle.vehicleId);
    }
}

// Stub implementations for required methods
std::vector<VehicleController::VehicleConflict> VehicleController::detectUpcomingConflicts() const {
    return std::vector<VehicleConflict>();
}

bool VehicleController::shouldVehicleWaitForConflictResolution(int vehicleId, const std::vector<VehicleConflict>& conflicts) const {
    return false;
}

bool VehicleController::resolveConflictThroughNegotiation(const VehicleConflict& conflict) {
    return true;
}

void VehicleController::preventDeadlockThroughCoordination() {
    // Basic implementation
}

bool VehicleController::isVehicleMoving(int vehicleId) const {
    const Auto* vehicle = getVehicle(vehicleId);
    return vehicle && vehicle->state == VehicleState::MOVING;
}

bool VehicleController::hasVehicleArrived(int vehicleId) const {
    const Auto* vehicle = getVehicle(vehicleId);
    return vehicle && vehicle->state == VehicleState::ARRIVED;
}

std::vector<int> VehicleController::getVehiclesAtPosition(const Point& position, float radius) const {
    std::vector<int> nearbyVehicles;
    for (const Auto& vehicle : vehicles) {
        if (vehicle.position.distanceTo(position) <= radius) {
            nearbyVehicles.push_back(vehicle.vehicleId);
        }
    }
    return nearbyVehicles;
}

// Additional stub implementations for required methods
void VehicleController::removeVehicle(int vehicleId) {}
void VehicleController::assignNewRandomTarget(int vehicleId) {}
bool VehicleController::isVehicleAtTarget(int vehicleId) const { return false; }
void VehicleController::setVehicleTarget(int vehicleId, const Point& targetPosition) {}
void VehicleController::setVehicleTarget(int vehicleId, int targetNodeId) {}
bool VehicleController::replanPathIfBlocked(int vehicleId) { return false; }
void VehicleController::clearPath(int vehicleId) {}
bool VehicleController::isPathBlocked(int vehicleId) const { return false; }
bool VehicleController::findAlternativePath(int vehicleId) { return false; }
int VehicleController::findCommonJunctionInPaths(const Auto& vehicle1, const Auto& vehicle2) const { return -1; }
float VehicleController::estimateTimeToJunction(const Auto& vehicle, int junctionId) const { return 0.0f; }
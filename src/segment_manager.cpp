
#include "segment_manager.h"
#include <algorithm>
#include <iostream>

SegmentManager::SegmentManager(PathSystem* pathSys) : pathSystem(pathSys) {}

bool SegmentManager::canVehicleEnterSegment(int segmentId, int vehicleId) const {
    PathSegment* segment = pathSystem->getSegment(segmentId);
    if (!segment) return false;
    
    // Vehicle can enter if segment is free or already occupied by this vehicle
    return !segment->isOccupied || segment->occupiedByVehicleId == vehicleId;
}

bool SegmentManager::reserveSegment(int segmentId, int vehicleId) {
    PathSegment* segment = pathSystem->getSegment(segmentId);
    if (!segment) return false;
    
    // Check if segment can be reserved
    if (segment->isOccupied && segment->occupiedByVehicleId != vehicleId) {
        return false;
    }
    
    // Reserve segment
    segment->isOccupied = true;
    segment->occupiedByVehicleId = vehicleId;
    vehicleToSegment[vehicleId] = segmentId;
    
    std::cout << "Vehicle " << vehicleId << " reserved segment " << segmentId << std::endl;
    return true;
}

void SegmentManager::releaseSegment(int segmentId, int vehicleId) {
    PathSegment* segment = pathSystem->getSegment(segmentId);
    if (!segment) return;
    
    // Only release if this vehicle actually occupies it
    if (segment->isOccupied && segment->occupiedByVehicleId == vehicleId) {
        segment->isOccupied = false;
        segment->occupiedByVehicleId = -1;
        vehicleToSegment.erase(vehicleId);
        
        // Process any queued vehicles for this segment
        processQueue(segmentId);
        
        std::cout << "Vehicle " << vehicleId << " released segment " << segmentId << std::endl;
    }
}

void SegmentManager::addToQueue(int segmentId, int vehicleId) {
    PathSegment* segment = pathSystem->getSegment(segmentId);
    if (!segment) return;
    
    // Check if vehicle is already in queue
    auto& queue = segment->queuedVehicles;
    if (std::find(queue.begin(), queue.end(), vehicleId) == queue.end()) {
        queue.push_back(vehicleId);
    }
}

void SegmentManager::removeFromQueue(int segmentId, int vehicleId) {
    PathSegment* segment = pathSystem->getSegment(segmentId);
    if (!segment) return;
    
    auto& queue = segment->queuedVehicles;
    queue.erase(std::remove(queue.begin(), queue.end(), vehicleId), queue.end());
}

void SegmentManager::processQueue(int segmentId) {
    PathSegment* segment = pathSystem->getSegment(segmentId);
    if (!segment || segment->isOccupied || segment->queuedVehicles.empty()) return;
    
    // Try to assign segment to first vehicle in queue
    int nextVehicleId = segment->queuedVehicles.front();
    segment->queuedVehicles.erase(segment->queuedVehicles.begin());
    
    if (reserveSegment(segmentId, nextVehicleId)) {
        std::cout << "Segment " << segmentId << " assigned to queued vehicle " << nextVehicleId << std::endl;
    } else {
        // If reservation failed, put vehicle back at front of queue
        segment->queuedVehicles.insert(segment->queuedVehicles.begin(), nextVehicleId);
    }
}

void SegmentManager::updateQueues() {
    // Process all segment queues
    for (const auto& segment : pathSystem->getSegments()) {
        if (!segment.isOccupied && !segment.queuedVehicles.empty()) {
            processQueue(segment.segmentId);
        }
    }
}

int SegmentManager::getVehicleSegment(int vehicleId) const {
    auto it = vehicleToSegment.find(vehicleId);
    return (it != vehicleToSegment.end()) ? it->second : -1;
}

void SegmentManager::removeVehicle(int vehicleId) {
    // Release any occupied segment
    int currentSegment = getVehicleSegment(vehicleId);
    if (currentSegment != -1) {
        releaseSegment(currentSegment, vehicleId);
    }

    // Remove from all queues
    for (const auto& segment : pathSystem->getSegments()) {
        removeFromQueue(segment.segmentId, vehicleId);
    }

    vehicleToSegment.erase(vehicleId);
}

std::vector<int> SegmentManager::findAvailablePath(int startNodeId, int endNodeId, int vehicleId) const {
    if (startNodeId == endNodeId) {
        return {};
    }

    // Find a path considering current segment occupancy
    std::vector<int> blockedSegments;
    
    // Collect currently occupied segments (except by this vehicle)
    for (const auto& segment : pathSystem->getSegments()) {
        if (segment.isOccupied && segment.occupiedByVehicleId != vehicleId) {
            blockedSegments.push_back(segment.segmentId);
        }
    }
    
    // Try to find path avoiding blocked segments
    std::vector<int> path = pathSystem->findPath(startNodeId, endNodeId, blockedSegments);
    
    // If no path found with blocked segments, try without restrictions
    if (path.empty()) {
        path = pathSystem->findPath(startNodeId, endNodeId, {});
    }
    
    return path;
}

std::vector<int> SegmentManager::findOptimalPath(int startNodeId, int endNodeId, int vehicleId) const {
    // Always return optimal path regardless of current occupancy
    return pathSystem->findPath(startNodeId, endNodeId, {});
}

bool SegmentManager::isPathClear(const std::vector<int>& path, int vehicleId) const {
    for (int segmentId : path) {
        if (!canVehicleEnterSegment(segmentId, vehicleId)) {
            return false;
        }
    }
    return true;
}

std::vector<int> SegmentManager::getOccupiedSegments() const {
    std::vector<int> occupied;
    
    for (const auto& segment : pathSystem->getSegments()) {
        if (segment.isOccupied) {
            occupied.push_back(segment.segmentId);
        }
    }
    
    return occupied;
}

void SegmentManager::printSegmentStatus() const {
    std::cout << "=== Segment Status ===" << std::endl;
    for (const auto& segment : pathSystem->getSegments()) {
        std::cout << "Segment " << segment.segmentId << ": ";
        if (segment.isOccupied) {
            std::cout << "OCCUPIED by vehicle " << segment.occupiedByVehicleId;
        } else {
            std::cout << "FREE";
        }
        
        if (!segment.queuedVehicles.empty()) {
            std::cout << " (Queue: ";
            for (size_t i = 0; i < segment.queuedVehicles.size(); i++) {
                std::cout << segment.queuedVehicles[i];
                if (i < segment.queuedVehicles.size() - 1) std::cout << ", ";
            }
            std::cout << ")";
        }
        std::cout << std::endl;
    }
    std::cout << "===================" << std::endl;
}

// Stub implementations for complex methods
float SegmentManager::estimatePathTime(const std::vector<int>& path, int vehicleId) const { return 0.0f; }
float SegmentManager::estimateWaitTime(int segmentId, int vehicleId) const { return 1.0f; }
bool SegmentManager::shouldWaitOrReroute(int currentNodeId, int targetNodeId, int blockedSegmentId, int vehicleId) const { return true; }
bool SegmentManager::isCurvePoint(int nodeId) const { return false; }
std::vector<int> SegmentManager::getCombinedCurveSegments(int nodeId) const { return {}; }
SegmentManager::NodeType SegmentManager::getNodeType(int nodeId) const { return NodeType::REGULAR; }
bool SegmentManager::isJunctionNode(int nodeId) const { return false; }
bool SegmentManager::isWaitingNode(int nodeId) const { return false; }
bool SegmentManager::isCurveNode(int nodeId) const { return false; }
std::vector<SegmentManager::ConflictInfo> SegmentManager::detectPotentialConflicts(int vehicleId, const std::vector<int>& plannedPath) const { return {}; }
bool SegmentManager::shouldWaitAtWaitingNode(int vehicleId, const std::vector<ConflictInfo>& conflicts) const { return false; }
std::vector<int> SegmentManager::findVehiclesApproachingJunction(int junctionId, int excludeVehicleId, float timeWindow) const { return {}; }
bool SegmentManager::isJunctionCurrentlyOccupied(int junctionId, int excludeVehicleId) const { return false; }
bool SegmentManager::hasOpposingTraffic(int junctionId, int vehicleId) const { return false; }
bool SegmentManager::negotiatePassage(int vehicleId, int junctionId, const std::vector<int>& conflictingVehicles) const { return true; }
bool SegmentManager::isTJunction(int nodeId) const { return false; }
bool SegmentManager::canUseEvasionRoute(int currentNodeId, int targetNodeId, int blockedSegmentId, int vehicleId) const { return false; }
std::vector<int> SegmentManager::findEvasionRoute(int currentNodeId, int targetNodeId, int blockedSegmentId, int vehicleId) const { return {}; }
bool SegmentManager::handleTJunctionConflict(int currentNodeId, int targetNodeId, int blockedSegmentId, int vehicleId) const { return true; }
int SegmentManager::findConflictingVehicle(int currentNodeId, int vehicleId) const { return -1; }
bool SegmentManager::vehiclesWantOppositeDirections(int currentNodeId, int vehicleId1, int vehicleId2) const { return false; }
bool SegmentManager::shouldUseEvasionSegment(int currentNodeId, int vehicleId, int conflictingVehicleId) const { return false; }
int SegmentManager::findEvasionSegment(int currentNodeId, int blockedSegmentId) const { return -1; }
int SegmentManager::getVehicleWaitingNode(int currentNodeId, int vehicleId) const { return -1; }
bool SegmentManager::isDeadlockSituation(int nodeId, int vehicleId, int otherVehicleId) const { return false; }
bool SegmentManager::hasConflictingTJunctionReservation(int tJunctionId, int vehicleId, int requestedSegmentId) const { return false; }
bool SegmentManager::detectDeadlock(const std::vector<int>& segmentsToReserve, int vehicleId) const { return false; }
bool SegmentManager::detectCircularWait(int startVehicleId, int currentVehicleId, std::set<int>& checkedVehicles) const { return false; }
bool SegmentManager::resolveDeadlock(int vehicleId, int blockedSegmentId) { return true; }
std::set<int> SegmentManager::findDeadlockCycle(int startVehicleId) const { return {}; }
bool SegmentManager::findCycleRecursive(int startVehicleId, int currentVehicleId, std::set<int>& visited, std::vector<int>& path, std::set<int>& cycleVehicles) const { return false; }
void SegmentManager::clearDeadlockQueues(const std::set<int>& deadlockVehicles) {}
bool SegmentManager::isVehicleWaitingForOurSegments(int waitingVehicleId, int ourVehicleId) const { return false; }
bool SegmentManager::segmentsConflict(int seg1, int seg2, int vehicle1, int vehicle2) const { return false; }
std::vector<int> SegmentManager::getConsolidatedSegmentGroup(int segmentId) const { return {segmentId}; }
void SegmentManager::checkAndAddConnectedSegments(int nodeId, std::vector<int>& toProcess, std::set<int>& processed) const {}

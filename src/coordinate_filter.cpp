
#include "coordinate_filter.h"
#include <algorithm>
#include <cmath>
#include <iostream>

CoordinateFilter::CoordinateFilter(float threshold, float timeout, int maxOutliers)
    : outlierThreshold(threshold), validityTimeout(timeout), maxConsecutiveOutliers(maxOutliers) {
}

std::vector<Point> CoordinateFilter::filterAndSmooth(const std::vector<Point>& newDetections, 
                                                    const std::vector<std::string>& colors) {
    // Zuerst abgelaufene Punkte entfernen
    removeExpiredPoints();
    
    // Neue Erkennungen verarbeiten
    for (size_t i = 0; i < newDetections.size() && i < colors.size(); i++) {
        updatePoint(newDetections[i], colors[i]);
    }
    
    // Stabilität der Punkte prüfen
    updatePointStability();
    
    // Nur stabile und gültige Punkte zurückgeben
    std::vector<Point> result;
    for (const auto& fp : filteredPoints) {
        if (fp.isValid && fp.isStable) {
            result.push_back(fp.point);
        }
    }
    
    return result;
}

void CoordinateFilter::updatePoint(const Point& newPoint, const std::string& color) {
    FilteredPoint* closest = findClosestExistingPoint(newPoint, color);
    
    if (closest != nullptr) {
        // Existierenden Punkt gefunden
        float distance = newPoint.distanceTo(closest->point);
        float maxMovementPerFrame = 150.0f; // Maximale Bewegung pro Frame - erlaubt normale Fahrzeugbewegung
        
        if (distance > maxMovementPerFrame) {
            // Zu große Bewegung (Teleportation) - als Ausreißer behandeln
            closest->consecutiveOutliers++;
            if (closest->consecutiveOutliers >= maxConsecutiveOutliers) {
                // Zu viele Ausreißer - Punkt als ungültig markieren
                closest->isValid = false;
                std::cout << "Punkt " << color << " wegen Teleportation deaktiviert" << std::endl;
            }
        } else if (isOutlier(newPoint, *closest)) {
            // Normaler Ausreißer erkannt
            closest->consecutiveOutliers++;
            if (closest->consecutiveOutliers >= maxConsecutiveOutliers) {
                closest->isValid = false;
                std::cout << "Punkt " << color << " wegen zu vieler Ausreißer deaktiviert" << std::endl;
            }
        } else {
            // Gültige Aktualisierung
            closest->point = newPoint;
            closest->lastUpdate = std::chrono::steady_clock::now();
            closest->consecutiveOutliers = 0;  // Ausreißer-Counter zurücksetzen
            closest->isValid = true;
        }
    } else {
        // Neuen Punkt hinzufügen
        filteredPoints.emplace_back(newPoint, color);
        std::cout << "Neuer gefilteter Punkt hinzugefügt: " << color << std::endl;
    }
}

void CoordinateFilter::removeExpiredPoints() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto& fp : filteredPoints) {
        if (fp.isValid) {
            auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - fp.lastUpdate);
            if (timeDiff.count() > validityTimeout * 1000) {
                fp.isValid = false;
                std::cout << "Punkt " << fp.color << " wegen Timeout deaktiviert" << std::endl;
            }
        }
    }
    
    // Ungültige Punkte komplett entfernen (um Speicher freizugeben)
    filteredPoints.erase(
        std::remove_if(filteredPoints.begin(), filteredPoints.end(),
                      [now, this](const FilteredPoint& fp) {
                          if (!fp.isValid) {
                              auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(now - fp.lastUpdate);
                              return timeDiff.count() > validityTimeout * 2; // Nach doppelter Timeout-Zeit komplett entfernen
                          }
                          return false;
                      }),
        filteredPoints.end());
}

bool CoordinateFilter::isOutlier(const Point& newPoint, const FilteredPoint& existing) const {
    float distance = newPoint.distanceTo(existing.point);
    return distance > outlierThreshold;
}

FilteredPoint* CoordinateFilter::findClosestExistingPoint(const Point& newPoint, const std::string& color) {
    FilteredPoint* closest = nullptr;
    float minDistance = outlierThreshold;
    
    for (auto& fp : filteredPoints) {
        if (fp.color == color) {
            float distance = newPoint.distanceTo(fp.point);
            if (distance < minDistance) {
                minDistance = distance;
                closest = &fp;
            }
        }
    }
    
    return closest;
}

size_t CoordinateFilter::getActivePointCount() const {
    return std::count_if(filteredPoints.begin(), filteredPoints.end(),
                        [](const FilteredPoint& fp) { return fp.isValid; });
}

void CoordinateFilter::updatePointStability() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto& fp : filteredPoints) {
        if (fp.isValid && !fp.isStable) {
            auto timeSinceCreation = std::chrono::duration_cast<std::chrono::milliseconds>(now - fp.creationTime);
            if (timeSinceCreation.count() >= fp.stabilityRequirement * 1000) {
                fp.isStable = true;
                std::cout << "Punkt " << fp.color << " ist jetzt stabil und bereit für Fahrzeugerkennung" << std::endl;
            }
        }
    }
}

void CoordinateFilter::clearAll() {
    filteredPoints.clear();
}

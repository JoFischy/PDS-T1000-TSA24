#include "coordinate_filter.h"
#include <algorithm>
#include <cmath>
#include <iostream>

CoordinateFilter::CoordinateFilter(float radius, float timeout, int minDetections, int maxRecent, float movement)
    : detectionRadius(radius), validityTimeout(timeout), 
      minDetectionsForStability(minDetections), maxRecentDetections(maxRecent),
      movementThreshold(movement) {
}

std::vector<Point> CoordinateFilter::filterAndSmooth(const std::vector<Point>& newDetections, 
                                                    const std::vector<std::string>& colors) {
    // Abgelaufene Punkte entfernen
    removeExpiredPoints();

    // Neue Erkennungen verarbeiten
    for (size_t i = 0; i < newDetections.size() && i < colors.size(); i++) {
        processDetection(newDetections[i], colors[i]);
    }

    // Nur stabile und gültige Punkte zurückgeben
    std::vector<Point> result;

    // Heck-Punkte hinzufügen (maximal 1 pro Heck-Typ 1,2,3,4)
    std::map<std::string, bool> heckNumbersAdded;
    for (const auto& [color, fp] : stablePoints) {
        if (fp.isValid && fp.isStable) {
            std::string partType = getVehiclePartType(color);
            if (partType == "heck") {
                std::string heckNumber = extractHeckNumber(color);
                if (heckNumbersAdded.find(heckNumber) == heckNumbersAdded.end()) {
                    // Point mit originalem Farbnamen erstellen
                    Point filteredPoint = fp.point;
                    filteredPoint.type = PointType::IDENTIFICATION;
                    filteredPoint.color = color; // Originale Farbe beibehalten (Heck1, Heck2, etc.)
                    result.push_back(filteredPoint);
                    heckNumbersAdded[heckNumber] = true;
                    std::cout << "Heck-Punkt " << heckNumber << " (" << color << ") hinzugefügt" << std::endl;
                } else {
                    std::cout << "Heck-Punkt " << heckNumber << " bereits hinzugefügt - " << color << " ignoriert" << std::endl;
                }
            }
        }
    }

    // Front-Punkte hinzufügen (maximal 4)
    int frontCount = 0;
    for (const auto& [color, fp] : stablePoints) {
        if (fp.isValid && fp.isStable && frontCount < 4) {
            std::string partType = getVehiclePartType(color);
            if (partType == "front") {
                // Point mit originalem Farbnamen erstellen
                Point filteredPoint = fp.point;
                filteredPoint.type = PointType::FRONT;
                filteredPoint.color = color; // "Front"
                result.push_back(filteredPoint);
                frontCount++;
                std::cout << "Front-Punkt (" << color << ") hinzugefügt" << std::endl;
            }
        }
    }

    return result;
}

void CoordinateFilter::processDetection(const Point& newPoint, const std::string& color) {
    auto it = stablePoints.find(color);

    if (it != stablePoints.end()) {
        // Existierender Punkt gefunden
        FilteredPoint& fp = it->second;

        // Prüfen ob neue Detektion im erlaubten Bereich ist
        if (fp.isStable && !isWithinMovementThreshold(fp.point, newPoint)) {
            // Zu große Bewegung - als Ausreißer ignorieren
            std::cout << "Ausreißer ignoriert für " << color << " (zu große Bewegung)" << std::endl;
            return;
        }

        // Neue Detektion zur Liste hinzufügen
        fp.recentDetections.push_back(newPoint);
        fp.totalDetections++;
        fp.lastUpdate = std::chrono::steady_clock::now();

        // Alte Detektionen entfernen (nur letzte X behalten)
        if (fp.recentDetections.size() > maxRecentDetections) {
            fp.recentDetections.erase(fp.recentDetections.begin());
        }

        // Cluster-Zentrum berechnen und Punkt aktualisieren
        fp.point = calculateClusterCenter(fp.recentDetections);

        // Stabilität prüfen
        updatePointStability(fp);

    } else {
        // Prüfen ob bereits zu viele Punkte dieses Typs existieren
        std::string partType = getVehiclePartType(color);

        if (partType == "heck") {
            // Für Heck-Punkte: Nur EINEN Punkt pro Heck-Typ (1,2,3,4) erlauben
            // Prüfen ob bereits ein Heck-Punkt mit derselben Nummer existiert
            for (const auto& [existingColor, fp] : stablePoints) {
                if (getVehiclePartType(existingColor) == "heck") {
                    // Extrahiere Heck-Nummer aus beiden Farben
                    std::string newHeckType = extractHeckNumber(color);
                    std::string existingHeckType = extractHeckNumber(existingColor);

                    if (newHeckType == existingHeckType && (fp.isValid || fp.isStable)) {
                        std::cout << "Heck-Punkt " << newHeckType << " bereits aktiv (" << existingColor 
                                  << ") - neue Detektion " << color << " ignoriert" << std::endl;
                        return;
                    }
                }
            }

            // Zusätzlich: Maximal 4 Heck-Punkte insgesamt
            int activeHeckCount = 0;
            for (const auto& [col, fp] : stablePoints) {
                if (getVehiclePartType(col) == "heck" && (fp.isValid || fp.isStable)) {
                    activeHeckCount++;
                }
            }

            if (activeHeckCount >= 4) {
                std::cout << "Bereits 4 Heck-Punkte aktiv - neue Detektion " << color << " ignoriert" << std::endl;
                return;
            }

        } else if (partType == "front") {
            // Für Front-Punkte: Maximal 4 erlauben
            int activeFrontCount = 0;
            for (const auto& [col, fp] : stablePoints) {
                if (getVehiclePartType(col) == "front" && (fp.isValid || fp.isStable)) {
                    activeFrontCount++;
                }
            }

            if (activeFrontCount >= 4) {
                std::cout << "Bereits 4 Front-Punkte aktiv - neue Detektion ignoriert" << std::endl;
                return;
            }
        }

        // Neuen Punkt erstellen
        stablePoints[color] = FilteredPoint(newPoint, color);
        std::cout << "Neuer " << partType << "-Punkt erkannt: " << color << std::endl;
    }
}

void CoordinateFilter::removeExpiredPoints() {
    auto now = std::chrono::steady_clock::now();

    auto it = stablePoints.begin();
    while (it != stablePoints.end()) {
        FilteredPoint& fp = it->second;
        auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - fp.lastUpdate);

        if (timeDiff.count() > validityTimeout * 1000) {
            std::cout << "Punkt " << fp.color << " wegen Timeout entfernt" << std::endl;
            it = stablePoints.erase(it);
        } else {
            ++it;
        }
    }
}

void CoordinateFilter::updatePointStability(FilteredPoint& fp) {
    // Punkt wird stabil wenn genug konsistente Detektionen vorhanden sind
    if (fp.totalDetections >= minDetectionsForStability) {
        // Prüfen ob alle letzten Detektionen im Stabilitätsradius liegen
        bool allWithinRadius = true;
        for (const Point& detection : fp.recentDetections) {
            if (fp.point.distanceTo(detection) > fp.stabilityRadius) {
                allWithinRadius = false;
                break;
            }
        }

        if (allWithinRadius && !fp.isStable) {
            fp.isStable = true;
            fp.isValid = true;
            fp.consecutiveValidDetections = fp.recentDetections.size();
            std::cout << "Punkt " << fp.color << " ist jetzt stabil nach " 
                      << fp.totalDetections << " Detektionen" << std::endl;
        } else if (allWithinRadius) {
            fp.isValid = true;
            fp.consecutiveValidDetections++;
        }
    }
}

Point CoordinateFilter::calculateClusterCenter(const std::vector<Point>& detections) const {
    if (detections.empty()) {
        return Point();
    }

    float sumX = 0.0f, sumY = 0.0f;
    for (const Point& p : detections) {
        sumX += p.x;
        sumY += p.y;
    }

    return Point(sumX / detections.size(), sumY / detections.size());
}

bool CoordinateFilter::isWithinMovementThreshold(const Point& oldPos, const Point& newPos) const {
    return oldPos.distanceTo(newPos) <= movementThreshold;
}

std::string CoordinateFilter::getVehiclePartType(const std::string& color) const {
    if (color.find("Heck") == 0) {
        return "heck";
    } else if (color.find("Front") == 0) {
        return "front";
    }
    return color;
}

std::string CoordinateFilter::extractHeckNumber(const std::string& color) const {
    if (color.find("Heck") == 0) {
        // Extract number from "Heck1", "Heck2", etc.
        std::string numberPart = color.substr(4); // Skip "Heck"
        return numberPart.empty() ? "0" : numberPart;
    }
    return "";
}

size_t CoordinateFilter::getActivePointCount() const {
    return std::count_if(stablePoints.begin(), stablePoints.end(),
                        [](const auto& pair) { return pair.second.isValid; });
}

void CoordinateFilter::clearAll() {
    stablePoints.clear();
}
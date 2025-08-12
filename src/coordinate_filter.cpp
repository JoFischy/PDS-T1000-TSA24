#include "coordinate_filter.h"
#include <algorithm>
#include <cmath>
#include <iostream>

CoordinateFilter::CoordinateFilter(float radius, float timeout, int minDetections, int maxRecent, float movement,
                                           float predTime, int maxMissed, float smoothing)
    : detectionRadius(radius), validityTimeout(timeout), 
      minDetectionsForStability(minDetections), maxRecentDetections(maxRecent),
      movementThreshold(movement), predictionTime(predTime), 
      maxMissedDetections(maxMissed), motionSmoothingFactor(smoothing) {
}

std::vector<Point> CoordinateFilter::filterAndSmooth(const std::vector<Point>& newDetections, 
                                                    const std::vector<std::string>& colors) {
    // Abgelaufene Punkte entfernen
    removeExpiredPoints();

    // Neue Erkennungen verarbeiten
    for (size_t i = 0; i < newDetections.size() && i < colors.size(); i++) {
        processDetection(newDetections[i], colors[i]);
    }

    // Generate predictions for missing points
    generatePredictedPoints();

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
                } else {
                    continue;
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
                filteredPoint.color = fp.color; // Verwende gespeicherte originale Farbe
                result.push_back(filteredPoint);
                frontCount++;
            }
        }
    }

    return result;
}

void CoordinateFilter::processDetection(const Point& newPoint, const std::string& color) {
    std::string partType = getVehiclePartType(color);
    std::string actualKey = color; // Standard: verwende die originale Farbe als Schlüssel

    // Für Front-Punkte: Spezielle Behandlung, da alle als "Front" kommen
    if (partType == "front") {
        // Suche nach existierendem Front-Punkt in der Nähe
        std::string nearbyFrontKey = "";
        for (const auto& [existingColor, fp] : stablePoints) {
            if (getVehiclePartType(existingColor) == "front" && 
                fp.point.distanceTo(newPoint) <= detectionRadius) {
                nearbyFrontKey = existingColor;
                break;
            }
        }

        if (!nearbyFrontKey.empty()) {
            // Verwende den existierenden Schlüssel
            actualKey = nearbyFrontKey;
        } else {
            // Erstelle neuen eindeutigen Schlüssel für Front-Punkt
            int frontIndex = 1;
            while (stablePoints.find("Front_" + std::to_string(frontIndex)) != stablePoints.end()) {
                frontIndex++;
            }
            actualKey = "Front_" + std::to_string(frontIndex);

            // Prüfe maximale Anzahl Front-Punkte (alle, nicht nur stabile)
            int activeFrontCount = 0;
            for (const auto& [col, fp] : stablePoints) {
                if (getVehiclePartType(col) == "front") {
                    activeFrontCount++;
                }
            }

            if (activeFrontCount >= 4) {
                std::cout << "Bereits 4 Front-Punkte aktiv - neue Detektion ignoriert" << std::endl;
                return;
            }
        }
    }

    auto it = stablePoints.find(actualKey);

    if (it != stablePoints.end()) {
        // Existierender Punkt gefunden
        FilteredPoint& fp = it->second;

        // Prüfen ob neue Detektion im erlaubten Bereich ist
        if (fp.isStable && !isWithinMovementThreshold(fp.point, newPoint)) {
            // Zu große Bewegung - als Ausreißer ignorieren
            std::cout << "Ausreißer ignoriert für " << actualKey << " (zu große Bewegung)" << std::endl;
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

        // Vorherige Position für Motion-Tracking speichern
        Point oldPosition = fp.point;

        // Cluster-Zentrum berechnen und Punkt aktualisieren
        fp.point = calculateClusterCenter(fp.recentDetections);

        // Motion Model aktualisieren (Geschwindigkeit, Beschleunigung)
        updateMotionModel(fp, fp.point);

        // Reset missed detections counter
        fp.missedDetections = 0;

        // Stabilität prüfen
        updatePointStability(fp);

    } else {
        // Für Heck-Punkte: Spezielle Validierung
        if (partType == "heck") {
            // Prüfen ob bereits ein Heck-Punkt mit derselben Nummer existiert
            for (const auto& [existingColor, fp] : stablePoints) {
                if (getVehiclePartType(existingColor) == "heck") {
                    std::string newHeckType = extractHeckNumber(color);
                    std::string existingHeckType = extractHeckNumber(existingColor);

                    if (newHeckType == existingHeckType && (fp.isValid || fp.isStable)) {
                        std::cout << "Heck-Punkt " << newHeckType << " bereits aktiv (" << existingColor 
                                  << ") - neue Detektion " << color << " ignoriert" << std::endl;
                        return;
                    }
                }
            }

            // Maximal 4 Heck-Punkte insgesamt
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
        }

        // Neuen Punkt erstellen
        stablePoints[actualKey] = FilteredPoint(newPoint, color); // Originale Farbe im FilteredPoint speichern
        std::cout << "Neuer " << partType << "-Punkt erkannt: " << actualKey << " (original: " << color << ")" << std::endl;
    }
}

void CoordinateFilter::removeExpiredPoints() {
    auto now = std::chrono::steady_clock::now();

    auto it = stablePoints.begin();
    while (it != stablePoints.end()) {
        FilteredPoint& fp = it->second;
        auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - fp.lastUpdate);

        // Increment missed detections counter
        if (timeDiff.count() > 100) { // 100ms ohne neue Detektion
            fp.missedDetections++;
        }

        // Remove point if too many detections missed or timeout
        bool shouldRemove = (timeDiff.count() > validityTimeout * 1000) || 
                           (fp.missedDetections > maxMissedDetections);

        if (shouldRemove) {
            std::cout << "Punkt " << fp.color << " entfernt (Timeout: " 
                      << (timeDiff.count() > validityTimeout * 1000) 
                      << ", Missed: " << fp.missedDetections << ")" << std::endl;
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

void CoordinateFilter::updateMotionModel(FilteredPoint& fp, const Point& newPosition) {
    auto now = std::chrono::steady_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - fp.lastUpdate);
    float deltaTime = timeDiff.count() / 1000.0f; // Convert to seconds

    if (deltaTime > 0 && fp.totalDetections > 1) {
        // Berechne neue Geschwindigkeit
        Point newVelocity;
        newVelocity.x = (newPosition.x - fp.predictedPosition.x) / deltaTime;
        newVelocity.y = (newPosition.y - fp.predictedPosition.y) / deltaTime;

        // Berechne neue Beschleunigung
        Point newAcceleration;
        newAcceleration.x = (newVelocity.x - fp.velocity.x) / deltaTime;
        newAcceleration.y = (newVelocity.y - fp.velocity.y) / deltaTime;

        // Glätte Geschwindigkeit und Beschleunigung mit exponentieller Filterung
        if (fp.hasPrediction) {
            fp.velocity.x = motionSmoothingFactor * fp.velocity.x + (1.0f - motionSmoothingFactor) * newVelocity.x;
            fp.velocity.y = motionSmoothingFactor * fp.velocity.y + (1.0f - motionSmoothingFactor) * newVelocity.y;
            fp.acceleration.x = motionSmoothingFactor * fp.acceleration.x + (1.0f - motionSmoothingFactor) * newAcceleration.x;
            fp.acceleration.y = motionSmoothingFactor * fp.acceleration.y + (1.0f - motionSmoothingFactor) * newAcceleration.y;
        } else {
            fp.velocity = newVelocity;
            fp.acceleration = newAcceleration;
            fp.hasPrediction = true;
        }

        // Begrenze extreme Werte
        const float maxVelocity = 1000.0f; // pixel/sec
        const float maxAcceleration = 2000.0f; // pixel/sec²
        
        fp.velocity.x = std::max(-maxVelocity, std::min(maxVelocity, fp.velocity.x));
        fp.velocity.y = std::max(-maxVelocity, std::min(maxVelocity, fp.velocity.y));
        fp.acceleration.x = std::max(-maxAcceleration, std::min(maxAcceleration, fp.acceleration.x));
        fp.acceleration.y = std::max(-maxAcceleration, std::min(maxAcceleration, fp.acceleration.y));
    }

    fp.predictedPosition = newPosition;
}

Point CoordinateFilter::predictNextPosition(const FilteredPoint& fp, float deltaTime) const {
    if (!fp.hasPrediction) {
        return fp.point;
    }

    // Kinematische Gleichung: s = s0 + v*t + 0.5*a*t²
    Point predicted;
    predicted.x = fp.point.x + fp.velocity.x * deltaTime + 0.5f * fp.acceleration.x * deltaTime * deltaTime;
    predicted.y = fp.point.y + fp.velocity.y * deltaTime + 0.5f * fp.acceleration.y * deltaTime * deltaTime;
    
    return predicted;
}

void CoordinateFilter::generatePredictedPoints() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto& [color, fp] : stablePoints) {
        if (fp.isStable && fp.hasPrediction && fp.missedDetections > 0) {
            auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - fp.lastUpdate);
            float deltaTime = timeDiff.count() / 1000.0f;
            
            // Verwende Vorhersage nur für eine begrenzte Zeit
            if (deltaTime < predictionTime && deltaTime > 0) {
                fp.predictedPosition = predictNextPosition(fp, deltaTime);
                
                // Update the point with predicted position for smoother tracking
                fp.point = fp.predictedPosition;
                
                std::cout << "Punkt " << color << " vorhergesagt bei (" 
                          << fp.predictedPosition.x << ", " << fp.predictedPosition.y 
                          << ") nach " << deltaTime << "s" << std::endl;
            }
        }
    }
}

size_t CoordinateFilter::getActivePointCount() const {
    return std::count_if(stablePoints.begin(), stablePoints.end(),
                        [](const auto& pair) { return pair.second.isValid; });
}

void CoordinateFilter::clearAll() {
    stablePoints.clear();
}
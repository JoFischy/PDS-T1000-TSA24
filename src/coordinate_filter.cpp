
#include "coordinate_filter.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>

CoordinateFilter::CoordinateFilter(float threshold, float timeout, int maxOutliers, int requiredDet)
    : outlierThreshold(threshold), validityTimeout(timeout), maxConsecutiveOutliers(maxOutliers), 
      requiredDetections(requiredDet) {
}

std::vector<Point> CoordinateFilter::filterAndSmooth(const std::vector<Point>& newDetections, 
                                                    const std::vector<std::string>& colors) {
    // Zuerst abgelaufene Punkte entfernen
    removeExpiredPoints();
    
    // Neue Erkennungen verarbeiten
    for (size_t i = 0; i < newDetections.size() && i < colors.size(); i++) {
        updatePoint(newDetections[i], colors[i]);
    }
    
    // Fahrzeugteil-Limits durchsetzen (vor Stabilitätsprüfung!)
    enforceVehiclePartLimits();
    
    // Stabilität der Punkte prüfen
    updatePointStability();
    
    // Fahrzeugteil-Limits durchsetzen
    enforceVehiclePartLimits();
    
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
            closest->consecutiveValidDetections = 0; // Gültige Erkennungen zurücksetzen
            if (closest->consecutiveOutliers >= maxConsecutiveOutliers) {
                // Zu viele Ausreißer - Punkt als ungültig markieren
                closest->isValid = false;
                closest->isStable = false;
                std::cout << "Punkt " << color << " wegen Teleportation deaktiviert" << std::endl;
            }
        } else if (isOutlier(newPoint, *closest)) {
            // Normaler Ausreißer erkannt
            closest->consecutiveOutliers++;
            closest->consecutiveValidDetections = 0; // Gültige Erkennungen zurücksetzen
            if (closest->consecutiveOutliers >= maxConsecutiveOutliers) {
                closest->isValid = false;
                closest->isStable = false;
                std::cout << "Punkt " << color << " wegen zu vieler Ausreißer deaktiviert" << std::endl;
            }
        } else {
            // Gültige Aktualisierung
            closest->point = newPoint;
            closest->lastUpdate = std::chrono::steady_clock::now();
            closest->consecutiveOutliers = 0;  // Ausreißer-Counter zurücksetzen
            closest->consecutiveValidDetections++; // Gültige Erkennung zählen
            
            // Punkt wird erst nach mehreren konsistenten Erkennungen als gültig betrachtet
            if (closest->consecutiveValidDetections >= closest->requiredConsecutiveDetections) {
                closest->isValid = true;
                if (!closest->isStable) {
                    std::cout << "Punkt " << color << " nach " << closest->consecutiveValidDetections 
                              << " konsistenten Erkennungen als gültig markiert" << std::endl;
                }
            }
        }
    } else {
        // Vor dem Hinzufügen eines neuen Punktes prüfen, ob bereits ein Punkt dieses Typs existiert
        std::string partType = getVehiclePartType(color);
        
        if (partType == "heck") {
            // Bei Heck-Teilen: Prüfen ob bereits ein Punkt mit dieser Farbe existiert
            bool alreadyExists = false;
            for (const auto& fp : filteredPoints) {
                if (fp.color == color && (fp.isValid || fp.isStable)) {
                    alreadyExists = true;
                    break;
                }
            }
            
            if (alreadyExists) {
                std::cout << "Neuer " << color << " Punkt ignoriert (bereits ein aktiver Punkt vorhanden)" << std::endl;
                return; // Punkt nicht hinzufügen
            }
        } else if (partType == "front") {
            // Bei Front-Teilen: Zählen wie viele bereits aktiv sind
            int activeFrontCount = 0;
            for (const auto& fp : filteredPoints) {
                if (getVehiclePartType(fp.color) == "front" && (fp.isValid || fp.isStable)) {
                    activeFrontCount++;
                }
            }
            
            if (activeFrontCount >= 4) {
                std::cout << "Neuer front Punkt ignoriert (bereits 4 aktive front Punkte vorhanden)" << std::endl;
                return; // Punkt nicht hinzufügen
            }
        }
        
        // Neuen Punkt hinzufügen - anfangs nicht gültig!
        FilteredPoint newFP(newPoint, color);
        newFP.requiredConsecutiveDetections = requiredDetections;
        filteredPoints.push_back(newFP);
        std::cout << "Neuer Punkt erkannt: " << color << " (benötigt " << requiredDetections 
                  << " konsistente Erkennungen für Gültigkeit)" << std::endl;
    }
}

void CoordinateFilter::removeExpiredPoints() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto& fp : filteredPoints) {
        if (fp.isValid) {
            auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - fp.lastUpdate);
            if (timeDiff.count() > validityTimeout * 1000) {
                fp.isValid = false;
                fp.isStable = false;
                std::cout << "Punkt " << fp.color << " wegen Timeout deaktiviert" << std::endl;
            }
        }
    }
    
    // Punkte entfernen, die nie gültig wurden oder schon lange ungültig sind
    filteredPoints.erase(
        std::remove_if(filteredPoints.begin(), filteredPoints.end(),
                      [now, this](const FilteredPoint& fp) {
                          auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(now - fp.lastUpdate);
                          
                          // Schnelleres Entfernen von Punkten, die nie gültig wurden (Störpunkte)
                          if (!fp.isValid && fp.consecutiveValidDetections < fp.requiredConsecutiveDetections) {
                              return timeDiff.count() > validityTimeout * 0.5; // Störpunkte nach halber Timeout-Zeit entfernen
                          }
                          
                          // Normale ungültige Punkte nach doppelter Timeout-Zeit entfernen
                          if (!fp.isValid) {
                              return timeDiff.count() > validityTimeout * 2;
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
            // Punkt wird stabil wenn er gültig ist UND die Mindestzeit erreicht hat
            if (timeSinceCreation.count() >= fp.stabilityRequirement * 1000) {
                fp.isStable = true;
                std::cout << "Punkt " << fp.color << " ist jetzt stabil und bereit für Fahrzeugerkennung "
                          << "(nach " << fp.consecutiveValidDetections << " Erkennungen)" << std::endl;
            }
        }
    }
}

void CoordinateFilter::clearAll() {
    filteredPoints.clear();
}

std::string CoordinateFilter::getVehiclePartType(const std::string& color) const {
    if (color.find("heck") != std::string::npos) {
        return "heck";
    } else if (color.find("front") != std::string::npos) {
        return "front";
    }
    return color; // Fallback für andere Farben
}

void CoordinateFilter::enforceVehiclePartLimits() {
    std::map<std::string, std::vector<FilteredPoint*>> heckParts; // heck1, heck2, etc.
    std::vector<FilteredPoint*> frontParts;
    
    // Alle Punkte nach Typ gruppieren (auch die noch nicht stabilen aber gültigen)
    for (auto& fp : filteredPoints) {
        if (fp.isValid) { // Nur gültige Punkte berücksichtigen
            std::string partType = getVehiclePartType(fp.color);
            
            if (partType == "heck") {
                heckParts[fp.color].push_back(&fp);
            } else if (partType == "front") {
                frontParts.push_back(&fp);
            }
        }
    }
    
    // Für jeden Heck-Typ: maximal 1 behalten
    for (auto& [heckType, points] : heckParts) {
        if (points.size() > 1) {
            // Sortiere nach Qualität (Stabilität > consecutive detections > Alter)
            std::sort(points.begin(), points.end(), 
                     [](const FilteredPoint* a, const FilteredPoint* b) {
                         // Stabile Punkte haben Priorität
                         if (a->isStable != b->isStable) {
                             return a->isStable > b->isStable;
                         }
                         // Dann nach Anzahl gültiger Erkennungen
                         if (a->consecutiveValidDetections != b->consecutiveValidDetections) {
                             return a->consecutiveValidDetections > b->consecutiveValidDetections;
                         }
                         // Schließlich ältere bevorzugen
                         return a->creationTime < b->creationTime;
                     });
            
            // Alle außer dem besten entfernen
            for (size_t i = 1; i < points.size(); i++) {
                points[i]->isValid = false;
                points[i]->isStable = false;
                std::cout << "Überflüssiger " << heckType << " Punkt entfernt (zu viele vom gleichen Typ, "
                          << points[i]->consecutiveValidDetections << " Erkennungen)" << std::endl;
            }
        }
    }
    
    // Für Front-Teile: maximal 4 behalten
    if (frontParts.size() > 4) {
        // Sortiere nach Qualität
        std::sort(frontParts.begin(), frontParts.end(), 
                 [](const FilteredPoint* a, const FilteredPoint* b) {
                     if (a->isStable != b->isStable) {
                         return a->isStable > b->isStable;
                     }
                     if (a->consecutiveValidDetections != b->consecutiveValidDetections) {
                         return a->consecutiveValidDetections > b->consecutiveValidDetections;
                     }
                     return a->creationTime < b->creationTime;
                 });
        
        // Alle außer den besten 4 entfernen
        for (size_t i = 4; i < frontParts.size(); i++) {
            frontParts[i]->isValid = false;
            frontParts[i]->isStable = false;
            std::cout << "Überflüssiger front Punkt entfernt (mehr als 4 aktiv, "
                      << frontParts[i]->consecutiveValidDetections << " Erkennungen)" << std::endl;
        }
    }
}

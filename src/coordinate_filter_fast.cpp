#include "coordinate_filter.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

/**
 * SCHNELLE VERSION des Koordinatenfilters
 * Entfernt alle geschwindigkeitshemmenden Mechanismen:
 * - Keine Kalman-Filter/Bewegungsmodelle
 * - Keine Geschwindigkeitsvorhersagen  
 * - Keine komplexe Stabilititätsprüfung
 * - Minimale Validierung nur für grobe Fehler
 * - Direkter Durchgang für maximale Geschwindigkeit
 */

class FastCoordinateFilter : public CoordinateFilter {
public:
    FastCoordinateFilter() : CoordinateFilter(50.0f, 2.0f, 1, 3, 200.0f, 0.1f, 2, 0.3f) {
        // Minimale Initialisierung für maximale Performance
    }

    std::vector<Point> filterAndSmooth(const std::vector<Point>& newDetections, 
                                      const std::vector<std::string>& colors) {
        std::vector<Point> result;
        
        // DIREKTER DURCHGANG - keine komplexe Filterung
        for (size_t i = 0; i < newDetections.size() && i < colors.size(); i++) {
            Point fastPoint = newDetections[i];
            
            // Nur grundlegende Validierung
            if (isValidCoordinate(fastPoint)) {
                // Setze Punkttyp basierend auf Farbe
                if (colors[i].find("Front") == 0) {
                    fastPoint.type = PointType::FRONT;
                } else if (colors[i].find("Heck") == 0) {
                    fastPoint.type = PointType::IDENTIFICATION;
                }
                
                fastPoint.color = colors[i];
                result.push_back(fastPoint);
            }
        }
        
        // Nur einfache Duplikatsprüfung für Heck-Punkte
        result = removeDuplicateHeckPoints(result);
        
        // Limitiere Front-Punkte auf maximal 4
        result = limitFrontPoints(result, 4);
        
        return result;
    }

private:
    bool isValidCoordinate(const Point& point) {
        // Nur grundlegende Koordinaten-Validierung
        return (point.x >= 0.0f && point.x <= 1000.0f && 
                point.y >= 0.0f && point.y <= 1000.0f);
    }
    
    std::vector<Point> removeDuplicateHeckPoints(const std::vector<Point>& points) {
        std::vector<Point> result;
        std::map<std::string, bool> heckNumbersSeen;
        
        for (const Point& p : points) {
            if (p.color.find("Heck") == 0) {
                std::string heckNumber = p.color.substr(4); // "Heck1" -> "1"
                if (heckNumbersSeen.find(heckNumber) == heckNumbersSeen.end()) {
                    heckNumbersSeen[heckNumber] = true;
                    result.push_back(p);
                }
            } else {
                result.push_back(p);
            }
        }
        
        return result;
    }
    
    std::vector<Point> limitFrontPoints(const std::vector<Point>& points, int maxFront) {
        std::vector<Point> frontPoints;
        std::vector<Point> otherPoints;
        
        // Trenne Front und andere Punkte
        for (const Point& p : points) {
            if (p.color.find("Front") == 0) {
                frontPoints.push_back(p);
            } else {
                otherPoints.push_back(p);
            }
        }
        
        // Limitiere Front-Punkte
        if (static_cast<int>(frontPoints.size()) > maxFront) {
            frontPoints.resize(maxFront);
        }
        
        // Kombiniere wieder
        std::vector<Point> result = otherPoints;
        result.insert(result.end(), frontPoints.begin(), frontPoints.end());
        
        return result;
    }
};

// Factory-Funktion für schnellen Filter
std::unique_ptr<CoordinateFilter> createFastCoordinateFilter() {
    return std::make_unique<FastCoordinateFilter>();
}

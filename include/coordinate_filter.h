
#ifndef COORDINATE_FILTER_H
#define COORDINATE_FILTER_H

#include "point.h"
#include <vector>
#include <chrono>
#include <string>

struct FilteredPoint {
    Point point;
    std::chrono::steady_clock::time_point lastUpdate;
    std::chrono::steady_clock::time_point creationTime;
    bool isValid;
    bool isStable;  // Punkt ist stabil genug für Fahrzeugerkennung
    std::string color;
    int consecutiveOutliers;
    float stabilityRequirement;  // Sekunden bis Punkt als stabil gilt
    
    FilteredPoint() : isValid(false), isStable(false), consecutiveOutliers(0), stabilityRequirement(0.5f) {}
    FilteredPoint(const Point& p, const std::string& c) 
        : point(p), lastUpdate(std::chrono::steady_clock::now()), 
          creationTime(std::chrono::steady_clock::now()),
          isValid(true), isStable(false), color(c), consecutiveOutliers(0), stabilityRequirement(0.5f) {}
};

class CoordinateFilter {
private:
    std::vector<FilteredPoint> filteredPoints;
    float outlierThreshold;        // Maximale Distanz für gültige Updates
    float validityTimeout;         // Sekunden bis Punkt ungültig wird
    int maxConsecutiveOutliers;    // Max Anzahl Ausreißer bevor Punkt ungültig

public:
    CoordinateFilter(float threshold = 80.0f, float timeout = 3.0f, int maxOutliers = 3);
    
    // Hauptfunktion: Filtert neue Erkennungen und gibt geglättete Punkte zurück
    std::vector<Point> filterAndSmooth(const std::vector<Point>& newDetections, 
                                      const std::vector<std::string>& colors);
    
    // Hilfsfunktionen
    void updatePoint(const Point& newPoint, const std::string& color);
    void removeExpiredPoints();
    bool isOutlier(const Point& newPoint, const FilteredPoint& existing) const;
    FilteredPoint* findClosestExistingPoint(const Point& newPoint, const std::string& color);
    
    // Getter/Setter
    void setOutlierThreshold(float threshold) { outlierThreshold = threshold; }
    void setValidityTimeout(float timeout) { validityTimeout = timeout; }
    float getOutlierThreshold() const { return outlierThreshold; }
    float getValidityTimeout() const { return validityTimeout; }
    
    // Stabilität und Debug-Informationen
    void updatePointStability();
    size_t getActivePointCount() const;
    void clearAll();
};

#endif

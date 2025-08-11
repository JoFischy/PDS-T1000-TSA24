
#ifndef COORDINATE_FILTER_H
#define COORDINATE_FILTER_H

#include "point.h"
#include <vector>
#include <chrono>
#include <string>
#include <map>

struct FilteredPoint {
    Point point;
    std::chrono::steady_clock::time_point lastUpdate;
    std::chrono::steady_clock::time_point creationTime;
    bool isValid;
    bool isStable;
    std::string color;
    int consecutiveValidDetections;
    int totalDetections;
    float stabilityRadius;  // Radius in dem Detektionen als "gleich" gelten
    std::vector<Point> recentDetections;  // Letzte Detektionen für Clustering
    
    // Prediction/Motion tracking
    Point velocity;           // Geschwindigkeit in x,y
    Point acceleration;       // Beschleunigung in x,y
    Point predictedPosition;  // Vorhergesagte Position
    bool hasPrediction;       // Hat gültige Vorhersage
    int missedDetections;     // Anzahl verpasster Detektionen
    
    FilteredPoint() : isValid(false), isStable(false), consecutiveValidDetections(0), 
                     totalDetections(0), stabilityRadius(50.0f), velocity(0.0f, 0.0f),
                     acceleration(0.0f, 0.0f), predictedPosition(0.0f, 0.0f), 
                     hasPrediction(false), missedDetections(0) {}
    FilteredPoint(const Point& p, const std::string& c) 
        : point(p), lastUpdate(std::chrono::steady_clock::now()), 
          creationTime(std::chrono::steady_clock::now()),
          isValid(false), isStable(false), color(c), 
          consecutiveValidDetections(0), totalDetections(1), stabilityRadius(50.0f),
          velocity(0.0f, 0.0f), acceleration(0.0f, 0.0f), predictedPosition(p),
          hasPrediction(false), missedDetections(0) {
        recentDetections.push_back(p);
    }
};

class CoordinateFilter {
private:
    std::map<std::string, FilteredPoint> stablePoints;  // Ein Punkt pro Farbe
    float detectionRadius;         // Radius für neue Detektionen
    float validityTimeout;         // Sekunden bis Punkt ungültig wird
    int minDetectionsForStability; // Mindest-Detektionen für Stabilität
    int maxRecentDetections;       // Maximale Anzahl gespeicherter Detektionen für Clustering
    float movementThreshold;       // Maximale Bewegung pro Frame
    
    // Prediction parameters
    float predictionTime;          // Sekunden in die Zukunft vorhersagen
    int maxMissedDetections;       // Maximale verpasste Detektionen bevor Punkt entfernt wird
    float motionSmoothingFactor;   // Glättungsfaktor für Geschwindigkeit/Beschleunigung

public:
    CoordinateFilter(float radius = 80.0f, float timeout = 2.0f, 
                    int minDetections = 5, int maxRecent = 10, float movement = 120.0f,
                    float predTime = 0.1f, int maxMissed = 3, float smoothing = 0.7f);
    
    // Hauptfunktion: Filtert neue Erkennungen und gibt stabile Punkte zurück
    std::vector<Point> filterAndSmooth(const std::vector<Point>& newDetections, 
                                      const std::vector<std::string>& colors);
    
    // Hilfsfunktionen
    void processDetection(const Point& newPoint, const std::string& color);
    void removeExpiredPoints();
    void updatePointStability(FilteredPoint& fp);
    Point calculateClusterCenter(const std::vector<Point>& detections) const;
    bool isWithinMovementThreshold(const Point& oldPos, const Point& newPos) const;
    std::string getVehiclePartType(const std::string& color) const;
    std::string extractHeckNumber(const std::string& color) const;
    
    // Prediction methods
    void updateMotionModel(FilteredPoint& fp, const Point& newPosition);
    Point predictNextPosition(const FilteredPoint& fp, float deltaTime) const;
    void generatePredictedPoints();
    
    // Getter/Setter
    void setDetectionRadius(float radius) { detectionRadius = radius; }
    void setValidityTimeout(float timeout) { validityTimeout = timeout; }
    float getDetectionRadius() const { return detectionRadius; }
    float getValidityTimeout() const { return validityTimeout; }
    
    size_t getActivePointCount() const;
    void clearAll();
};

#endif

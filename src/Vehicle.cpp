#include "../include/Vehicle.h"
#include <cmath>

Vehicle::Vehicle(const std::string& vehicle_name, 
                 const std::string& front_color, const cv::Scalar& front_lower, const cv::Scalar& front_upper,
                 const std::string& rear_color, const cv::Scalar& rear_lower, const cv::Scalar& rear_upper)
    : name(vehicle_name), front_color_name(front_color), rear_color_name(rear_color),
      front_hsv_lower(front_lower), front_hsv_upper(front_upper),
      rear_hsv_lower(rear_lower), rear_hsv_upper(rear_upper) {
}

VehicleDetectionData Vehicle::detect(const cv::Mat& frame) {
    VehicleDetectionData data;
    data.vehicle_name = name;
    data.front_color = front_color_name;
    data.rear_color = rear_color_name;
    
    // Konvertiere zu HSV
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    
    // Erkenne vordere Farbe
    data.front_pos = find_color_center(hsv, front_hsv_lower, front_hsv_upper);
    data.has_front = is_valid_detection(data.front_pos);
    
    // Erkenne hintere Farbe
    data.rear_pos = find_color_center(hsv, rear_hsv_lower, rear_hsv_upper);
    data.has_rear = is_valid_detection(data.rear_pos);
    
    // Berechne Richtung wenn beide Farben erkannt
    if (data.has_front && data.has_rear) {
        data.angle_degrees = calculate_angle(data.front_pos, data.rear_pos);
        data.distance_pixels = cv::norm(data.front_pos - data.rear_pos);
        data.has_angle = true;
    }
    
    return data;
}

cv::Point2f Vehicle::find_color_center(const cv::Mat& hsv, const cv::Scalar& lower, const cv::Scalar& upper) {
    cv::Mat mask;
    cv::inRange(hsv, lower, upper, mask);
    
    // Rauschen entfernen
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
    
    // Finde Konturen
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    if (!contours.empty()) {
        // Finde größte Kontur
        double max_area = 0;
        int max_index = 0;
        for (size_t i = 0; i < contours.size(); i++) {
            double area = cv::contourArea(contours[i]);
            if (area > max_area) {
                max_area = area;
                max_index = i;
            }
        }
        
        // Mindestgröße prüfen
        if (max_area > 50) {
            cv::Moments M = cv::moments(contours[max_index]);
            return cv::Point2f(M.m10 / M.m00, M.m01 / M.m00);
        }
    }
    
    return cv::Point2f(-1, -1);  // Nicht gefunden
}

bool Vehicle::is_valid_detection(const cv::Point2f& point) {
    return point.x >= 0 && point.y >= 0;
}

float Vehicle::calculate_angle(const cv::Point2f& front, const cv::Point2f& rear) {
    float dx = front.x - rear.x;
    float dy = front.y - rear.y;
    
    // atan2(dx, -dy) für 0° = nach oben
    float angle_rad = std::atan2(dx, -dy);
    float angle_deg = angle_rad * 180.0f / M_PI;
    
    // Normalisiere zu 0-360°
    if (angle_deg < 0) {
        angle_deg += 360.0f;
    }
    
    return angle_deg;
}

void Vehicle::draw_detection(cv::Mat& frame, const VehicleDetectionData& data) {
    // Fahrzeugname oben links anzeigen
    static int name_offset = 0;
    cv::putText(frame, data.vehicle_name + ": " + 
                (data.has_angle ? std::to_string((int)data.angle_degrees) + "°" : "?"),
                cv::Point(10, 30 + name_offset * 25), 
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    name_offset = (name_offset + 1) % 4;  // Reset nach 4 Fahrzeugen
    
    // Zeichne erkannte Punkte
    if (data.has_front) {
        cv::circle(frame, data.front_pos, 10, cv::Scalar(0, 255, 0), 3);  // Grün für vorne
        cv::putText(frame, "V", cv::Point(data.front_pos.x + 15, data.front_pos.y), 
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
    }
    
    if (data.has_rear) {
        cv::circle(frame, data.rear_pos, 10, cv::Scalar(0, 0, 255), 3);   // Rot für hinten
        cv::putText(frame, "H", cv::Point(data.rear_pos.x + 15, data.rear_pos.y), 
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
    }
    
    // Zeichne Verbindungslinie und Richtungspfeil
    if (data.has_angle) {
        cv::line(frame, data.rear_pos, data.front_pos, cv::Scalar(255, 255, 0), 2);
        
        // Richtungspfeil am vorderen Punkt
        float angle_rad = data.angle_degrees * M_PI / 180.0f;
        cv::Point2f arrow_end(
            data.front_pos.x + 20 * std::sin(angle_rad),
            data.front_pos.y - 20 * std::cos(angle_rad)
        );
        cv::arrowedLine(frame, data.front_pos, arrow_end, cv::Scalar(255, 255, 0), 3);
    }
}

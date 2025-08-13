#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include "Vehicle.h"
#include "auto.h"
#include "point.h"
#include "path_system.h"
#include "vehicle_controller.h"

// Alternative: Einfaches Windows API Fenster (ohne Raylib Includes hier)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <commctrl.h>  // Für Trackbar-Controls

#pragma comment(lib, "comctl32.lib")  // Link Trackbar-Library

// Undefs für Konflikte mit Raylib
#ifdef Rectangle
#undef Rectangle
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Globale Variablen für Koordinaten-Update und Auto-Erkennung
static std::vector<DetectedObject> g_detected_objects;
static std::vector<Point> g_points;
static std::vector<Auto> g_detected_autos;
static std::mutex g_data_mutex;
static HWND g_test_window_hwnd = nullptr;
static float g_tolerance = 150.0f;
static HBITMAP g_backgroundBitmap = nullptr;

// Globale Variablen für PathSystem und VehicleController
static const PathSystem* g_path_system = nullptr;
static const VehicleController* g_vehicle_controller = nullptr;

// Kalibrierungsparameter für Koordinaten-Anpassung
static float g_x_scale = 1.0f;          // X-Skalierung (Standard: 1.0)
static float g_y_scale = 1.0f;          // Y-Skalierung (Standard: 1.0)
static float g_x_offset = 0.0f;         // X-Offset
static float g_y_offset = 0.0f;         // Y-Offset

// PathSystem und Punkte verwenden direkte 1920x1200 Koordinaten
// Keine Skalierung nötig da alles bereits im 1920x1200 Format ist

// Direkte Koordinaten-Mapping für Vollbild
Point mapToFullscreenCoordinates(float x, float y) {
    // Koordinaten sind bereits im 1920x1200 Format
    return Point(x, y);
}
static float g_x_curve = 0.0f;          // X-Kurvenkorrektur für Mitte/Rand-Problem
static float g_y_curve = 0.0f;          // Y-Kurvenkorrektur für Oben/Unten-Problem

// Trackbar-IDs für Kalibrierung
#define ID_TRACKBAR_X_SCALE    1001
#define ID_TRACKBAR_Y_SCALE    1002
#define ID_TRACKBAR_X_OFFSET   1003
#define ID_TRACKBAR_Y_OFFSET   1004
#define ID_TRACKBAR_X_CURVE    1005
#define ID_TRACKBAR_Y_CURVE    1006

// Trackbar-Handles (nicht verwendet, aber für zukünftige Erweiterungen definiert)
// static HWND g_trackbar_x_scale = nullptr;
// static HWND g_trackbar_y_scale = nullptr;
// static HWND g_trackbar_x_offset = nullptr;
// static HWND g_trackbar_y_offset = nullptr;
// static HWND g_trackbar_x_curve = nullptr;
// static HWND g_trackbar_y_curve = nullptr;

// Kalibrierungs-Fenster
static HWND g_calibration_window = nullptr;

// Konstanten für vollbild 1920x1200
static const int FULLSCREEN_WIDTH = 1920;
static const int FULLSCREEN_HEIGHT = 1200;

// Trackbar-Wert zu Float konvertieren
float trackbarToFloat(int trackbarValue, float minVal, float maxVal) {
    return minVal + (trackbarValue / 1000.0f) * (maxVal - minVal);
}

// Float-Wert zu Trackbar konvertieren
int floatToTrackbar(float value, float minVal, float maxVal) {
    return (int)((value - minVal) / (maxVal - minVal) * 1000.0f);
}

// Kalibrierungs-Fenster Message Handler
LRESULT CALLBACK CalibrationWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Common Controls initialisieren
            InitCommonControls();

            int y = 20;
            int labelWidth = 100;
            int trackbarWidth = 200;

            // X-Scale Trackbar (0.5 - 2.0)
            CreateWindowA("STATIC", "X-Scale:", WS_VISIBLE | WS_CHILD, 10, y, labelWidth, 20, hwnd, nullptr, nullptr, nullptr);
            HWND xScaleTrack = CreateWindowA(TRACKBAR_CLASSA, "", WS_VISIBLE | WS_CHILD | TBS_HORZ,
                                           labelWidth + 10, y, trackbarWidth, 30, hwnd, (HMENU)ID_TRACKBAR_X_SCALE, nullptr, nullptr);
            SendMessage(xScaleTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 1000));
            SendMessage(xScaleTrack, TBM_SETPOS, TRUE, floatToTrackbar(g_x_scale, 0.5f, 2.0f));
            y += 40;

            // Y-Scale Trackbar (0.5 - 2.0)
            CreateWindowA("STATIC", "Y-Scale:", WS_VISIBLE | WS_CHILD, 10, y, labelWidth, 20, hwnd, nullptr, nullptr, nullptr);
            HWND yScaleTrack = CreateWindowA(TRACKBAR_CLASSA, "", WS_VISIBLE | WS_CHILD | TBS_HORZ,
                                           labelWidth + 10, y, trackbarWidth, 30, hwnd, (HMENU)ID_TRACKBAR_Y_SCALE, nullptr, nullptr);
            SendMessage(yScaleTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 1000));
            SendMessage(yScaleTrack, TBM_SETPOS, TRUE, floatToTrackbar(g_y_scale, 0.5f, 2.0f));
            y += 40;

            // X-Offset Trackbar (-200 - +200)
            CreateWindowA("STATIC", "X-Offset:", WS_VISIBLE | WS_CHILD, 10, y, labelWidth, 20, hwnd, nullptr, nullptr, nullptr);
            HWND xOffsetTrack = CreateWindowA(TRACKBAR_CLASSA, "", WS_VISIBLE | WS_CHILD | TBS_HORZ,
                                             labelWidth + 10, y, trackbarWidth, 30, hwnd, (HMENU)ID_TRACKBAR_X_OFFSET, nullptr, nullptr);
            SendMessage(xOffsetTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 1000));
            SendMessage(xOffsetTrack, TBM_SETPOS, TRUE, floatToTrackbar(g_x_offset, -200.0f, 200.0f));
            y += 40;

            // Y-Offset Trackbar (-200 - +200)
            CreateWindowA("STATIC", "Y-Offset:", WS_VISIBLE | WS_CHILD, 10, y, labelWidth, 20, hwnd, nullptr, nullptr, nullptr);
            HWND yOffsetTrack = CreateWindowA(TRACKBAR_CLASSA, "", WS_VISIBLE | WS_CHILD | TBS_HORZ,
                                             labelWidth + 10, y, trackbarWidth, 30, hwnd, (HMENU)ID_TRACKBAR_Y_OFFSET, nullptr, nullptr);
            SendMessage(yOffsetTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 1000));
            SendMessage(yOffsetTrack, TBM_SETPOS, TRUE, floatToTrackbar(g_y_offset, -200.0f, 200.0f));
            y += 40;

            // X-Curve Trackbar (-0.5 - +0.5)
            CreateWindowA("STATIC", "X-Curve:", WS_VISIBLE | WS_CHILD, 10, y, labelWidth, 20, hwnd, nullptr, nullptr, nullptr);
            HWND xCurveTrack = CreateWindowA(TRACKBAR_CLASSA, "", WS_VISIBLE | WS_CHILD | TBS_HORZ,
                                            labelWidth + 10, y, trackbarWidth, 30, hwnd, (HMENU)ID_TRACKBAR_X_CURVE, nullptr, nullptr);
            SendMessage(xCurveTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 1000));
            SendMessage(xCurveTrack, TBM_SETPOS, TRUE, floatToTrackbar(g_x_curve, -0.5f, 0.5f));
            y += 40;

            // Y-Curve Trackbar (-0.5 - +0.5)
            CreateWindowA("STATIC", "Y-Curve:", WS_VISIBLE | WS_CHILD, 10, y, labelWidth, 20, hwnd, nullptr, nullptr, nullptr);
            HWND yCurveTrack = CreateWindowA(TRACKBAR_CLASSA, "", WS_VISIBLE | WS_CHILD | TBS_HORZ,
                                            labelWidth + 10, y, trackbarWidth, 30, hwnd, (HMENU)ID_TRACKBAR_Y_CURVE, nullptr, nullptr);
            SendMessage(yCurveTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 1000));
            SendMessage(yCurveTrack, TBM_SETPOS, TRUE, floatToTrackbar(g_y_curve, -0.5f, 0.5f));

            return 0;
        }
        case WM_HSCROLL: {
            if (LOWORD(wParam) == TB_THUMBTRACK || LOWORD(wParam) == TB_ENDTRACK) {
                int trackbarId = GetDlgCtrlID((HWND)lParam);
                int position = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);

                switch (trackbarId) {
                    case ID_TRACKBAR_X_SCALE:
                        g_x_scale = trackbarToFloat(position, 0.5f, 2.0f);
                        break;
                    case ID_TRACKBAR_Y_SCALE:
                        g_y_scale = trackbarToFloat(position, 0.5f, 2.0f);
                        break;
                    case ID_TRACKBAR_X_OFFSET:
                        g_x_offset = trackbarToFloat(position, -200.0f, 200.0f);
                        break;
                    case ID_TRACKBAR_Y_OFFSET:
                        g_y_offset = trackbarToFloat(position, -200.0f, 200.0f);
                        break;
                    case ID_TRACKBAR_X_CURVE:
                        g_x_curve = trackbarToFloat(position, -0.5f, 0.5f);
                        break;
                    case ID_TRACKBAR_Y_CURVE:
                        g_y_curve = trackbarToFloat(position, -0.5f, 0.5f);
                        break;
                }

                // Hauptfenster neu zeichnen
                if (g_test_window_hwnd) {
                    InvalidateRect(g_test_window_hwnd, nullptr, FALSE);
                }
            }
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            g_calibration_window = nullptr;
            return 0;
        case WM_DESTROY:
            return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// Kalibrierungs-Fenster erstellen
void createCalibrationWindow() {
    if (g_calibration_window != nullptr) return; // Schon offen

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    // Fensterklasse registrieren
    WNDCLASSA wc = {};
    wc.lpfnWndProc = CalibrationWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "CalibrationWindow";
    wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassA(&wc);

    // Verschiebles Fenster erstellen
    g_calibration_window = CreateWindowExA(
        0,
        "CalibrationWindow",
        "Koordinaten-Kalibrierung",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,  // WS_VISIBLE hinzugefügt
        100, 100, 350, 300,
        nullptr, nullptr, hInstance, nullptr
    );

    if (g_calibration_window) {
        ShowWindow(g_calibration_window, SW_SHOW);
        UpdateWindow(g_calibration_window);
    }
}

// Direkte Crop-zu-Vollbild Koordinaten-Transformation
void transformCropToFullscreen(float crop_x, float crop_y, float crop_width, float crop_height, float& fullscreen_x, float& fullscreen_y) {
    // Die crop_x und crop_y sind bereits Pixel-Koordinaten im Crop-Bereich
    // Diese werden direkt auf 1920x1200 Vollbild skaliert

    if (crop_width > 0 && crop_height > 0) {
        // Normalisiere auf 0.0 bis 1.0 basierend auf Crop-Größe
        float norm_x = crop_x / crop_width;
        float norm_y = crop_y / crop_height;

        // Skaliere direkt auf 1920x1200 Vollbild
        fullscreen_x = norm_x * FULLSCREEN_WIDTH;
        fullscreen_y = norm_y * FULLSCREEN_HEIGHT;
    } else {
        fullscreen_x = 0.0f;
        fullscreen_y = 0.0f;
    }
}

// Helper-Funktionen für Auto-Erkennung (wie in car_simulation.cpp)
void detectVehiclesInTestWindow() {
    g_detected_autos.clear();

    // Create vectors to separate identification and front points
    std::vector<size_t> identificationIndices;
    std::vector<size_t> frontIndices;

    for (size_t i = 0; i < g_points.size(); i++) {
        if (g_points[i].type == PointType::IDENTIFICATION) {
            identificationIndices.push_back(i);
        } else if (g_points[i].type == PointType::FRONT) {
            frontIndices.push_back(i);
        }
    }

    // Track which front points have already been used
    std::vector<bool> frontPointUsed(frontIndices.size(), false);

    // For each identification point, find the closest available front point within tolerance
    for (size_t idIdx : identificationIndices) {
        float bestDistance = g_tolerance + 1.0f;
        int bestFrontIdx = -1;
        int bestFrontVectorIdx = -1;

        // Check all front points
        for (size_t j = 0; j < frontIndices.size(); j++) {
            if (frontPointUsed[j]) continue;

            size_t frontIdx = frontIndices[j];
            float distance = g_points[idIdx].distanceTo(g_points[frontIdx]);

            if (distance <= g_tolerance && distance < bestDistance) {
                bestDistance = distance;
                bestFrontIdx = frontIdx;
                bestFrontVectorIdx = j;
            }
        }

        // If a valid pair was found, create a vehicle
        if (bestFrontIdx != -1) {
            frontPointUsed[bestFrontVectorIdx] = true;
            Auto detectedAuto(g_points[idIdx], g_points[bestFrontIdx]);
            g_detected_autos.push_back(detectedAuto);

            printf("Created vehicle %s: ID(%.2f, %.2f) + Front(%.2f, %.2f) = Center(%.2f, %.2f)\n",
                   g_points[idIdx].color.c_str(), 
                   g_points[idIdx].x, g_points[idIdx].y,
                   g_points[bestFrontIdx].x, g_points[bestFrontIdx].y,
                   detectedAuto.getCenter().x, detectedAuto.getCenter().y);
        } else {
            printf("No matching Front point found for %s\n", g_points[idIdx].color.c_str());
        }
    }
}

// Zeichne PathSystem-Knoten
void drawPathNodeGDI(HDC hdc, const PathNode& node) {
    // Verwende direkt die Knoten-Position (bereits im 1920x1200 Format)
    Point nodePos = mapToFullscreenCoordinates(node.position.x, node.position.y);

    // Verschiedene Farben für verschiedene Knotentypen
    COLORREF nodeColor = RGB(70, 130, 255); // Blau für normale Knoten
    COLORREF borderColor = RGB(0, 0, 0);    // Schwarzer Rand

    if (node.isWaitingNode) {
        nodeColor = RGB(255, 150, 0); // Orange für Warteknoten
        borderColor = RGB(200, 100, 0);
    }

    // Bestimme Knotentyp basierend auf Verbindungen
    int connectionCount = node.connectedSegments.size();
    if (connectionCount >= 4) {
        nodeColor = RGB(255, 0, 0);   // Rot für Kreuzungen (4+ Verbindungen)
        borderColor = RGB(200, 0, 0);
    } else if (connectionCount == 3) {
        nodeColor = RGB(255, 255, 0); // Gelb für T-Kreuzungen
        borderColor = RGB(200, 200, 0);
    }

    HBRUSH brush = CreateSolidBrush(nodeColor);
    HPEN pen = CreatePen(PS_SOLID, 2, borderColor);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    // Knoten zeichnen (größer als normale Punkte)
    int radius = 15;
    Ellipse(hdc, 
            static_cast<int>(nodePos.x - radius), 
            static_cast<int>(nodePos.y - radius),
            static_cast<int>(nodePos.x + radius), 
            static_cast<int>(nodePos.y + radius));

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    // Node-ID als Label
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));

    char nodeLabel[16];
    snprintf(nodeLabel, sizeof(nodeLabel), "%d", node.nodeId);
    TextOutA(hdc, static_cast<int>(nodePos.x + 20), static_cast<int>(nodePos.y - 10), 
             nodeLabel, strlen(nodeLabel));
}

// Zeichne PathSystem-Segment
void drawPathSegmentGDI(HDC hdc, const PathSegment& segment, const PathSystem& pathSystem) {
    const PathNode* startNode = pathSystem.getNode(segment.startNodeId);
    const PathNode* endNode = pathSystem.getNode(segment.endNodeId);

    if (!startNode || !endNode) return;

    // Verwende direkt die Knoten-Positionen (bereits im 1920x1200 Format)
    Point startPos = mapToFullscreenCoordinates(startNode->position.x, startNode->position.y);
    Point endPos = mapToFullscreenCoordinates(endNode->position.x, endNode->position.y);

    // Farbe basierend auf Belegung
    COLORREF segmentColor = RGB(150, 150, 150); // Grau für freie Segmente
    if (segment.isOccupied) {
        segmentColor = RGB(200, 50, 200); // Magenta für belegte Segmente
    }

    HPEN pen = CreatePen(PS_SOLID, 4, segmentColor);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    MoveToEx(hdc, static_cast<int>(startPos.x), static_cast<int>(startPos.y), nullptr);
    LineTo(hdc, static_cast<int>(endPos.x), static_cast<int>(endPos.y));

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

// Zeichne Fahrzeugroute
void drawVehicleRouteGDI(HDC hdc, const Auto& vehicle, const PathSystem& pathSystem) {
    if (!g_vehicle_controller) return;

    // Finde das entsprechende Vehicle im Controller
    const auto& vehicles = g_vehicle_controller->getVehicles();
    for (const auto& controllerVehicle : vehicles) {
        if (controllerVehicle.vehicleId == vehicle.getId()) {
            if (controllerVehicle.currentPath.empty()) continue;

            // Routenfarbe basierend auf Fahrzeug-ID
            COLORREF routeColors[] = {
                RGB(0, 255, 0),   // Grün
                RGB(255, 255, 0), // Gelb  
                RGB(255, 165, 0), // Orange
                RGB(128, 0, 128)  // Lila
            };
            COLORREF routeColor = routeColors[vehicle.getId() % 4];

            HPEN routePen = CreatePen(PS_SOLID, 6, routeColor);
            HGDIOBJ oldPen = SelectObject(hdc, routePen);

            // Zeichne Routensegmente
            for (size_t i = controllerVehicle.currentSegmentIndex; i < controllerVehicle.currentPath.size(); i++) {
                int segmentId = controllerVehicle.currentPath[i];
                const PathSegment* segment = pathSystem.getSegment(segmentId);

                if (segment) {
                    const PathNode* startNode = pathSystem.getNode(segment->startNodeId);
                    const PathNode* endNode = pathSystem.getNode(segment->endNodeId);

                    if (startNode && endNode) {
                        // Aktuelles Segment dicker zeichnen
                        if (i == controllerVehicle.currentSegmentIndex) {
                            SelectObject(hdc, oldPen);
                            DeleteObject(routePen);
                            routePen = CreatePen(PS_SOLID, 8, routeColor);
                            SelectObject(hdc, routePen);
                        }

                        // Verwende direkte Koordinaten für Vollbild
                        Point startPos = mapToFullscreenCoordinates(startNode->position.x, startNode->position.y);
                        Point endPos = mapToFullscreenCoordinates(endNode->position.x, endNode->position.y);

                        MoveToEx(hdc, static_cast<int>(startPos.x), 
                                static_cast<int>(startPos.y), nullptr);
                        LineTo(hdc, static_cast<int>(endPos.x), 
                               static_cast<int>(endPos.y));
                    }
                }
            }

            SelectObject(hdc, oldPen);
            DeleteObject(routePen);

            // Zeichne Zielknoten
            if (controllerVehicle.targetNodeId != -1) {
                const PathNode* targetNode = pathSystem.getNode(controllerVehicle.targetNodeId);
                if (targetNode) {
                    // Verwende direkte Koordinaten für Zielknoten
                    Point targetPos = mapToFullscreenCoordinates(targetNode->position.x, targetNode->position.y);

                    HBRUSH targetBrush = CreateSolidBrush(routeColor);
                    HGDIOBJ oldBrush = SelectObject(hdc, targetBrush);

                    int targetRadius = 20;
                    Ellipse(hdc, 
                            static_cast<int>(targetPos.x - targetRadius), 
                            static_cast<int>(targetPos.y - targetRadius),
                            static_cast<int>(targetPos.x + targetRadius), 
                            static_cast<int>(targetPos.y + targetRadius));

                    SelectObject(hdc, oldBrush);
                    DeleteObject(targetBrush);
                }
            }
            break;
        }
    }
}

// Zeichne einen Punkt (bereits in Vollbild-Koordinaten)
void drawPointGDI(HDC hdc, const Point& point, bool isSelected) {
    // Point ist bereits in 1920x1200 Koordinaten transformiert
    Point fullscreenPoint = point;

    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0)); // Schwarz
    if (isSelected) {
        brush = CreateSolidBrush(RGB(255, 0, 0)); // Rot für Auswahl
    }

    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    // Kreis zeichnen (Radius 12 wie im Original)
    int radius = 12;
    Ellipse(hdc, 
            static_cast<int>(fullscreenPoint.x - radius), 
            static_cast<int>(fullscreenPoint.y - radius),
            static_cast<int>(fullscreenPoint.x + radius), 
            static_cast<int>(fullscreenPoint.y + radius));

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    // Label zeichnen
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));

    if (!point.color.empty()) {
        if (point.color == "Front") {
            TextOutA(hdc, static_cast<int>(fullscreenPoint.x + 15), static_cast<int>(fullscreenPoint.y - 15), "FRONT", 5);
        } else if (point.color.find("Heck") == 0) {
            std::string label = point.color;
            std::transform(label.begin(), label.end(), label.begin(), ::toupper);
            TextOutA(hdc, static_cast<int>(fullscreenPoint.x + 15), static_cast<int>(fullscreenPoint.y - 15), label.c_str(), label.length());
        }
    }
}

// Zeichne ein Auto als kompakten Punkt mit Richtungspfeil
void drawAutoGDI(HDC hdc, const Auto& auto_) {
    if (!auto_.isValid()) return;

    Point center = auto_.getCenter();
    Point frontPoint = auto_.getFrontPoint();

    // Die Auto-Koordinaten sind bereits in 1920x1200 Vollbild-Koordinaten
    Point fullscreenCenter = center;
    Point fullscreenFront = frontPoint;

    // Berechne Richtungsvektor
    float dx = fullscreenFront.x - fullscreenCenter.x;
    float dy = fullscreenFront.y - fullscreenCenter.y;
    float length = sqrt(dx * dx + dy * dy);

    if (length > 0) {
        dx /= length;
        dy /= length;
    }

    // Auto-Mittelpunkt als grüner Kreis
    HBRUSH autoBrush = CreateSolidBrush(RGB(0, 255, 0));
    HGDIOBJ oldBrush = SelectObject(hdc, autoBrush);
    HPEN autoPen = CreatePen(PS_SOLID, 2, RGB(0, 180, 0));
    HGDIOBJ oldPen = SelectObject(hdc, autoPen);

    int radius = 12;
    Ellipse(hdc, 
            static_cast<int>(fullscreenCenter.x - radius), 
            static_cast<int>(fullscreenCenter.y - radius),
            static_cast<int>(fullscreenCenter.x + radius), 
            static_cast<int>(fullscreenCenter.y + radius));

    SelectObject(hdc, oldBrush);
    DeleteObject(autoBrush);

    // Richtungspfeil (kompakt)
    HPEN arrowPen = CreatePen(PS_SOLID, 4, RGB(255, 100, 0));
    SelectObject(hdc, arrowPen);

    float arrowLength = 25.0f;
    float arrowEndX = fullscreenCenter.x + dx * arrowLength;
    float arrowEndY = fullscreenCenter.y + dy * arrowLength;

    // Hauptlinie des Pfeils
    MoveToEx(hdc, static_cast<int>(fullscreenCenter.x), static_cast<int>(fullscreenCenter.y), nullptr);
    LineTo(hdc, static_cast<int>(arrowEndX), static_cast<int>(arrowEndY));

    // Pfeilspitze
    float arrowHeadLength = 8.0f;
    float arrowHeadAngle = 0.5f;

    float leftX = arrowEndX - (dx * cos(arrowHeadAngle) - dy * sin(arrowHeadAngle)) * arrowHeadLength;
    float leftY = arrowEndY - (dx * sin(arrowHeadAngle) + dy * cos(arrowHeadAngle)) * arrowHeadLength;
    float rightX = arrowEndX - (dx * cos(-arrowHeadAngle) - dy * sin(-arrowHeadAngle)) * arrowHeadLength;
    float rightY = arrowEndY - (dx * sin(-arrowHeadAngle) + dy * cos(-arrowHeadAngle)) * arrowHeadLength;

    MoveToEx(hdc, static_cast<int>(arrowEndX), static_cast<int>(arrowEndY), nullptr);
    LineTo(hdc, static_cast<int>(leftX), static_cast<int>(leftY));
    MoveToEx(hdc, static_cast<int>(arrowEndX), static_cast<int>(arrowEndY), nullptr);
    LineTo(hdc, static_cast<int>(rightX), static_cast<int>(rightY));

    SelectObject(hdc, oldPen);
    DeleteObject(autoPen);
    DeleteObject(arrowPen);
}

LRESULT CALLBACK TestWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // DOUBLE-BUFFERING für weniger Flackern
            RECT rect;
            GetClientRect(hwnd, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            // Erstelle Memory-DC für Double-Buffering
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
            HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

            // Weißer Hintergrund
            FillRect(memDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

            // Zeichne Hintergrundbild falls geladen
            if (g_backgroundBitmap) {
                HDC bgDC = CreateCompatibleDC(memDC);
                HGDIOBJ oldBgBitmap = SelectObject(bgDC, g_backgroundBitmap);

                BITMAP bmp;
                GetObject(g_backgroundBitmap, sizeof(BITMAP), &bmp);

                // Skaliere auf Fenstergröße
                SetStretchBltMode(memDC, HALFTONE);
                StretchBlt(memDC, 0, 0, width, height, 
                          bgDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

                SelectObject(bgDC, oldBgBitmap);
                DeleteDC(bgDC);
            }

            // Hole aktuelle Daten (Thread-safe)
            std::vector<Point> current_points;
            std::vector<Auto> current_autos;
            {
                std::lock_guard<std::mutex> lock(g_data_mutex);
                current_points = g_points;
                current_autos = g_detected_autos;
            }

            // Zeichne PathSystem-Elemente (falls verfügbar)
            if (g_path_system) {
                // Zeichne zuerst alle Segmente (Pfade)
                for (const auto& segment : g_path_system->getSegments()) {
                    drawPathSegmentGDI(memDC, segment, *g_path_system);
                }

                // Zeichne alle Knoten
                for (const auto& node : g_path_system->getNodes()) {
                    drawPathNodeGDI(memDC, node);
                }

                // Zeichne Fahrzeugrouten
                for (const Auto& auto_ : current_autos) {
                    drawVehicleRouteGDI(memDC, auto_, *g_path_system);
                }
            }

            // Zeichne erkannte Autos (berechnete Fahrzeuge mit Richtungspfeil)
            for (const Auto& auto_ : current_autos) {
                drawAutoGDI(memDC, auto_);
            }

            // UI-Info links oben (minimal wie im Original)
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, RGB(0, 0, 0));
            HFONT hFont = CreateFontA(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
                                     CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                     DEFAULT_PITCH | FF_SWISS, "Arial");
            HGDIOBJ oldFont = SelectObject(memDC, hFont);

            TextOutA(memDC, 10, 10, "PDS-T1000-TSA24 - ZWEITES FENSTER", 45);

            // Fahrzeug-Info
            int yOffset = 40;
            HFONT hInfoFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
                                         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                         DEFAULT_PITCH | FF_SWISS, "Arial");
            SelectObject(memDC, hInfoFont);

            // Draw vehicle information
            char vehicleInfo[256];
            snprintf(vehicleInfo, sizeof(vehicleInfo), "Vehicles: %zu", current_autos.size());
            TextOutA(memDC, 10, yOffset, vehicleInfo, strlen(vehicleInfo));
            yOffset += 20;

            // Draw detailed vehicle information
            for (size_t i = 0; i < current_autos.size(); i++) {
                const Auto& vehicle = current_autos[i];
                if (vehicle.isValid()) {
                    Point center = vehicle.getCenter();
                    float direction = vehicle.getDirection();

                    char detailInfo[256];
                    snprintf(detailInfo, sizeof(detailInfo), "Auto %d: (%.0f, %.0f) %.0f°", 
                            vehicle.getId(), center.x, center.y, direction);
                    TextOutA(memDC, 10, yOffset, detailInfo, strlen(detailInfo));
                    yOffset += 18;
                }
            }


            // Kopiere Memory-DC zum Bildschirm (Double-Buffer-Trick)
            BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

            // Aufräumen
            SelectObject(memDC, oldFont);
            SelectObject(memDC, oldBitmap);
            DeleteObject(hFont);
            DeleteObject(hInfoFont);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            // Mausklick für Routenauswahl
            int mouseX = LOWORD(lParam);
            int mouseY = HIWORD(lParam);

            if (g_path_system && g_vehicle_controller) {
                // Mauskoordinaten sind direkt im 1920x1200 Vollbild-Format
                Point clickPos(static_cast<float>(mouseX), static_cast<float>(mouseY));
                int nearestNodeId = g_path_system->findNearestNode(clickPos, 100.0f);

                if (nearestNodeId != -1) {
                    // Hole aktuelle Daten
                    std::vector<Auto> current_autos;
                    {
                        std::lock_guard<std::mutex> lock(g_data_mutex);
                        current_autos = g_detected_autos;
                    }

                    // Setze Ziel für das erste gültige Fahrzeug
                    for (const Auto& auto_ : current_autos) {
                        if (auto_.isValid()) {
                            int vehicleId = auto_.getId();
                            const_cast<VehicleController*>(g_vehicle_controller)->setVehicleTargetNode(vehicleId, nearestNodeId);

                            char message[256];
                            snprintf(message, sizeof(message), "Auto %d Route zu Knoten %d gesetzt\nKlick: (%.0f, %.0f)", 
                                    vehicleId, nearestNodeId, (double)mouseX, (double)mouseY);
                            MessageBoxA(hwnd, message, "Route geplant", MB_OK | MB_ICONINFORMATION);
                            break; // Nur für das erste Auto eine Route setzen
                        }
                    }
                } else {
                    char message[256];
                    snprintf(message, sizeof(message), "Kein Knoten gefunden bei (%.0f, %.0f)\nVersuche näher an einen Knoten zu klicken", 
                            (double)mouseX, (double)mouseY);
                    MessageBoxA(hwnd, message, "Kein Ziel gefunden", MB_OK | MB_ICONWARNING);
                }
            }
            return 0;
        }
        case WM_TIMER:
            // Weniger häufige automatische Updates (alle 200ms statt 100ms)
            InvalidateRect(hwnd, nullptr, FALSE); // FALSE = kein Hintergrund löschen
            return 0;
        case WM_CLOSE:
            KillTimer(hwnd, 1);
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

void createWindowsAPITestWindow() {
    std::thread([]() {
        // Warte kurz
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        HINSTANCE hInstance = GetModuleHandle(nullptr);

        // Fensterklasse registrieren
        WNDCLASSA wc = {};
        wc.lpfnWndProc = TestWindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = "TestWindow";
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClassA(&wc)) {
            std::cout << "Fensterklasse Registrierung fehlgeschlagen" << std::endl;
            return;
        }

        // Hauptmonitor Position ermitteln
        RECT primaryRect;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &primaryRect, 0);

        // Fenster erstellen (vollbild 1920x1200)
        HWND hwnd = CreateWindowExA(
            WS_EX_TOPMOST,             // Immer im Vordergrund
            "TestWindow",
            "PDS-T1000-TSA24 - ZWEITES FENSTER",
            WS_POPUP,                  // Vollbild ohne Rahmen
            0,                         // X Position 
            0,                         // Y Position  
            1920,                      // Breite (vollbild)
            1200,                      // Höhe (vollbild)
            nullptr,
            nullptr,
            hInstance,
            nullptr
        );

        if (!hwnd) {
            std::cout << "Fenster Erstellung fehlgeschlagen" << std::endl;
            return;
        }

        // Fenster-Handle global speichern
        g_test_window_hwnd = hwnd;

        // Lade Hintergrundbild - verwende BMP für Windows API
        g_backgroundBitmap = (HBITMAP)LoadImageA(nullptr, "assets/factory_layout.bmp", 
                                                IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        if (!g_backgroundBitmap) {
            // Versuche auch andere Pfade
            g_backgroundBitmap = (HBITMAP)LoadImageA(nullptr, "./factory_layout.bmp", 
                                                    IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        }
        if (!g_backgroundBitmap) {
            // Versuche relativen Pfad
            g_backgroundBitmap = (HBITMAP)LoadImageA(nullptr, "../assets/factory_layout.bmp", 
                                                    IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        }

        if (g_backgroundBitmap) {
            std::cout << "Hintergrundbild erfolgreich geladen für zweites Fenster!" << std::endl;
        } else {
            std::cout << "Warnung: Hintergrundbild konnte nicht geladen werden!" << std::endl;
        }

        ShowWindow(hwnd, SW_SHOW); // Vollbild anzeigen
        UpdateWindow(hwnd);

        // Timer für regelmäßige Updates (alle 200ms statt 100ms für weniger Flackern)
        SetTimer(hwnd, 1, 200, nullptr);

        std::cout << "Zweites Auto-Fenster maximiert auf Hauptmonitor erstellt!" << std::endl;

        // Automatisch Kalibrierungs-Fenster öffnen
        createCalibrationWindow();
        std::cout << "Kalibrierungs-Fenster automatisch geöffnet!" << std::endl;

        // Message Loop
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    }).detach();
}

// Setze PathSystem und VehicleController Referenzen
void setTestWindowPathSystem(const PathSystem* pathSystem, const VehicleController* vehicleController) {
    std::lock_guard<std::mutex> lock(g_data_mutex);
    g_path_system = pathSystem;
    g_vehicle_controller = vehicleController;

    // Fenster zur Neuzeichnung veranlassen
    if (g_test_window_hwnd != nullptr) {
        InvalidateRect(g_test_window_hwnd, nullptr, FALSE);
    }
}

// Update-Funktion für Live-Koordinaten und Auto-Erkennung
void updateTestWindowCoordinates(const std::vector<DetectedObject>& detected_objects) {
    // Thread-safe Update der globalen Koordinaten
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        g_detected_objects = detected_objects;

        // Konvertiere DetectedObjects zu Points mit Crop-zu-Vollbild Transformation
        g_points.clear();

        printf("Updating test window with %zu detected objects\n", detected_objects.size());

        // Convert detected objects to points with fullscreen coordinates
        for (const auto& obj : detected_objects) {
            printf("Processing object: color=%s, coords=(%.2f, %.2f), crop_size=(%.2f, %.2f)\n", 
                   obj.color.c_str(), obj.coordinates.x, obj.coordinates.y, obj.crop_width, obj.crop_height);

            float fullscreen_x, fullscreen_y;
            transformCropToFullscreen(obj.coordinates.x, obj.coordinates.y, 
                                    obj.crop_width, obj.crop_height, 
                                    fullscreen_x, fullscreen_y);

            // Überprüfe, ob die transformierten Koordinaten gültig sind
            if (fullscreen_x >= 0 && fullscreen_x <= FULLSCREEN_WIDTH && 
                fullscreen_y >= 0 && fullscreen_y <= FULLSCREEN_HEIGHT) {

                // Create point with correct type and color
                if (obj.color == "Front") {
                    g_points.emplace_back(fullscreen_x, fullscreen_y, PointType::FRONT, obj.color);
                    printf("Added Front point at (%.2f, %.2f)\n", fullscreen_x, fullscreen_y);
                } else if (obj.color.find("Heck") == 0) {
                    g_points.emplace_back(fullscreen_x, fullscreen_y, PointType::IDENTIFICATION, obj.color);
                    printf("Added %s point at (%.2f, %.2f)\n", obj.color.c_str(), fullscreen_x, fullscreen_y);
                }
            } else {
                printf("WARNING: Invalid coordinates after transformation: (%.2f, %.2f)\n", fullscreen_x, fullscreen_y);
            }
        }

        printf("Total points added: %zu\n", g_points.size());

        // Detect vehicles from the updated points
        detectVehiclesInTestWindow();

        printf("Total vehicles detected: %zu\n", g_detected_autos.size());
    }

    // Fenster zur Neuzeichnung veranlassen (falls es existiert)
    // WENIGER HÄUFIGE UPDATES für weniger Flackern
    if (g_test_window_hwnd != nullptr) {
        InvalidateRect(g_test_window_hwnd, nullptr, FALSE); // FALSE = kein Hintergrund löschen
    }
}

// Update-Funktion für erkannte Autos von der Hauptanwendung
void updateTestWindowVehicles(const std::vector<Auto>& vehicles) {
    std::lock_guard<std::mutex> lock(g_data_mutex);
    g_detected_autos = vehicles;

    // Fenster zur Neuzeichnung veranlassen
    if (g_test_window_hwnd != nullptr) {
        InvalidateRect(g_test_window_hwnd, nullptr, FALSE);
    }
}
#endif
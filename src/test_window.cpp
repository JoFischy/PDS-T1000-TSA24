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

// PathSystem Koordinaten-Skalierung
static const float ORIGINAL_WIDTH = 1920.0f;   // Original PathSystem Breite
static const float ORIGINAL_HEIGHT = 1200.0f;  // Original PathSystem Höhe

// Skalierungsfunktion für PathSystem-Koordinaten
Point scalePathSystemCoordinates(float originalX, float originalY, int windowWidth, int windowHeight) {
    float scaleX = windowWidth / ORIGINAL_WIDTH;
    float scaleY = windowHeight / ORIGINAL_HEIGHT;
    
    // Verwende das kleinere der beiden Skalierungen, um Proportionen zu bewahren
    float scale = std::min(scaleX, scaleY);
    
    float scaledX = originalX * scale;
    float scaledY = originalY * scale;
    
    // Zentriere das Bild
    float offsetX = (windowWidth - ORIGINAL_WIDTH * scale) / 2.0f;
    float offsetY = (windowHeight - ORIGINAL_HEIGHT * scale) / 2.0f;
    
    return Point(scaledX + offsetX, scaledY + offsetY);
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
    
    // Verschiebares Fenster erstellen
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

// Kalibrierte Koordinaten-Transformation
void transformCoordinates(float norm_x, float norm_y, int windowWidth, int windowHeight, float& window_x, float& window_y) {
    // Basis-Skalierung
    window_x = norm_x * windowWidth * g_x_scale;
    window_y = norm_y * windowHeight * g_y_scale;
    
    // Kurvenkorrektur für Mitte/Rand und Oben/Unten Probleme
    // X-Korrektur: Mitte ist gut, Ränder zu weit innen -> quadratische Korrektur
    float x_center_offset = (norm_x - 0.5f); // -0.5 bis +0.5
    window_x += x_center_offset * x_center_offset * g_x_curve * windowWidth;
    
    // Y-Korrektur: Oben ist gut, unten zu hoch -> lineare/quadratische Korrektur  
    float y_offset_factor = norm_y; // 0.0 bis 1.0
    window_y += y_offset_factor * g_y_curve * windowHeight;
    
    // Finale Verschiebung
    window_x += g_x_offset;
    window_y += g_y_offset;
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

        // If we found a suitable front point, create a vehicle
        if (bestFrontIdx != -1) {
            g_detected_autos.emplace_back(g_points[idIdx], g_points[bestFrontIdx]);
            frontPointUsed[bestFrontVectorIdx] = true;
        }
    }
}

// Zeichne PathSystem-Knoten
void drawPathNodeGDI(HDC hdc, const PathNode& node) {
    // Hole Fenster-Dimensionen für Skalierung
    RECT windowRect;
    if (!GetClientRect(g_test_window_hwnd, &windowRect)) {
        return; // Fehler beim Ermitteln der Fenstergröße
    }
    
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    
    // Skaliere die Knoten-Position
    Point scaledPos = scalePathSystemCoordinates(node.position.x, node.position.y, windowWidth, windowHeight);
    
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
            static_cast<int>(scaledPos.x - radius), 
            static_cast<int>(scaledPos.y - radius),
            static_cast<int>(scaledPos.x + radius), 
            static_cast<int>(scaledPos.y + radius));
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
    
    // Node-ID als Label
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    
    char nodeLabel[16];
    snprintf(nodeLabel, sizeof(nodeLabel), "%d", node.nodeId);
    TextOutA(hdc, static_cast<int>(scaledPos.x + 20), static_cast<int>(scaledPos.y - 10), 
             nodeLabel, strlen(nodeLabel));
}

// Zeichne PathSystem-Segment
void drawPathSegmentGDI(HDC hdc, const PathSegment& segment, const PathSystem& pathSystem) {
    const PathNode* startNode = pathSystem.getNode(segment.startNodeId);
    const PathNode* endNode = pathSystem.getNode(segment.endNodeId);
    
    if (!startNode || !endNode) return;
    
    // Hole Fenster-Dimensionen für Skalierung
    RECT windowRect;
    if (!GetClientRect(g_test_window_hwnd, &windowRect)) {
        return; // Fehler beim Ermitteln der Fenstergröße
    }
    
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    
    // Skaliere die Start- und End-Positionen
    Point scaledStart = scalePathSystemCoordinates(startNode->position.x, startNode->position.y, windowWidth, windowHeight);
    Point scaledEnd = scalePathSystemCoordinates(endNode->position.x, endNode->position.y, windowWidth, windowHeight);
    
    // Farbe basierend auf Belegung
    COLORREF segmentColor = RGB(150, 150, 150); // Grau für freie Segmente
    if (segment.isOccupied) {
        segmentColor = RGB(200, 50, 200); // Magenta für belegte Segmente
    }
    
    HPEN pen = CreatePen(PS_SOLID, 4, segmentColor);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    
    MoveToEx(hdc, static_cast<int>(scaledStart.x), static_cast<int>(scaledStart.y), nullptr);
    LineTo(hdc, static_cast<int>(scaledEnd.x), static_cast<int>(scaledEnd.y));
    
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
                        
                        // Hole Fenster-Dimensionen für Skalierung
                        RECT windowRect;
                        if (GetClientRect(g_test_window_hwnd, &windowRect)) {
                            int windowWidth = windowRect.right - windowRect.left;
                            int windowHeight = windowRect.bottom - windowRect.top;
                            
                            Point scaledStart = scalePathSystemCoordinates(startNode->position.x, startNode->position.y, windowWidth, windowHeight);
                            Point scaledEnd = scalePathSystemCoordinates(endNode->position.x, endNode->position.y, windowWidth, windowHeight);
                            
                            MoveToEx(hdc, static_cast<int>(scaledStart.x), 
                                    static_cast<int>(scaledStart.y), nullptr);
                            LineTo(hdc, static_cast<int>(scaledEnd.x), 
                                   static_cast<int>(scaledEnd.y));
                        }
                    }
                }
            }
            
            SelectObject(hdc, oldPen);
            DeleteObject(routePen);
            
            // Zeichne Zielknoten
            if (controllerVehicle.targetNodeId != -1) {
                const PathNode* targetNode = pathSystem.getNode(controllerVehicle.targetNodeId);
                if (targetNode) {
                    // Hole Fenster-Dimensionen für Skalierung
                    RECT windowRect;
                    if (GetClientRect(g_test_window_hwnd, &windowRect)) {
                        int windowWidth = windowRect.right - windowRect.left;
                        int windowHeight = windowRect.bottom - windowRect.top;
                        
                        Point scaledTarget = scalePathSystemCoordinates(targetNode->position.x, targetNode->position.y, windowWidth, windowHeight);
                        
                        HBRUSH targetBrush = CreateSolidBrush(routeColor);
                        HGDIOBJ oldBrush = SelectObject(hdc, targetBrush);
                        
                        int targetRadius = 20;
                        Ellipse(hdc, 
                                static_cast<int>(scaledTarget.x - targetRadius), 
                                static_cast<int>(scaledTarget.y - targetRadius),
                                static_cast<int>(scaledTarget.x + targetRadius), 
                                static_cast<int>(scaledTarget.y + targetRadius));
                        
                        SelectObject(hdc, oldBrush);
                        DeleteObject(targetBrush);
                    }
                }
            }
            break;
        }
    }
}

// Zeichne einen Punkt (wie in renderer.cpp)
void drawPointGDI(HDC hdc, const Point& point, bool isSelected) {
    // Hole Fenster-Dimensionen für Skalierung
    RECT windowRect;
    if (!GetClientRect(g_test_window_hwnd, &windowRect)) {
        return; // Fehler beim Ermitteln der Fenstergröße
    }
    
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    
    // Skaliere die Punkt-Position
    Point scaledPoint = scalePathSystemCoordinates(point.x, point.y, windowWidth, windowHeight);
    
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
            static_cast<int>(scaledPoint.x - radius), 
            static_cast<int>(scaledPoint.y - radius),
            static_cast<int>(scaledPoint.x + radius), 
            static_cast<int>(scaledPoint.y + radius));
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
    
    // Label zeichnen
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    
    if (!point.color.empty()) {
        if (point.color == "Front") {
            TextOutA(hdc, static_cast<int>(scaledPoint.x + 15), static_cast<int>(scaledPoint.y - 15), "FRONT", 5);
        } else if (point.color.find("Heck") == 0) {
            std::string label = point.color;
            std::transform(label.begin(), label.end(), label.begin(), ::toupper);
            TextOutA(hdc, static_cast<int>(scaledPoint.x + 15), static_cast<int>(scaledPoint.y - 15), label.c_str(), label.length());
        }
    }
}

// Zeichne ein Auto als kompakten Punkt mit Richtungspfeil
void drawAutoGDI(HDC hdc, const Auto& auto_) {
    if (!auto_.isValid()) return;

    Point center = auto_.getCenter();
    Point frontPoint = auto_.getFrontPoint();
    
    // Hole Fenster-Dimensionen für Skalierung
    RECT windowRect;
    if (!GetClientRect(g_test_window_hwnd, &windowRect)) {
        return; // Fehler beim Ermitteln der Fenstergröße
    }
    
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    
    // Raylib-Fenster-Dimensionen (Vollbild oder maximiert)
    int raylib_width = GetSystemMetrics(SM_CXSCREEN);
    int raylib_height = GetSystemMetrics(SM_CYSCREEN);
    
    // Einfache Skalierung: Raylib-Koordinaten auf Test-Fenster
    float scaleX = (float)windowWidth / raylib_width;
    float scaleY = (float)windowHeight / raylib_height;
    
    Point scaledCenter(center.x * scaleX, center.y * scaleY);
    Point scaledFront(frontPoint.x * scaleX, frontPoint.y * scaleY);

    // Berechne Richtungsvektor
    float dx = scaledFront.x - scaledCenter.x;
    float dy = scaledFront.y - scaledCenter.y;
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
            static_cast<int>(scaledCenter.x - radius), 
            static_cast<int>(scaledCenter.y - radius),
            static_cast<int>(scaledCenter.x + radius), 
            static_cast<int>(scaledCenter.y + radius));
    
    SelectObject(hdc, oldBrush);
    DeleteObject(autoBrush);

    // Richtungspfeil (kompakt)
    HPEN arrowPen = CreatePen(PS_SOLID, 4, RGB(255, 100, 0));
    SelectObject(hdc, arrowPen);
    
    float arrowLength = 25.0f;
    float arrowEndX = scaledCenter.x + dx * arrowLength;
    float arrowEndY = scaledCenter.y + dy * arrowLength;
    
    // Hauptlinie des Pfeils
    MoveToEx(hdc, static_cast<int>(scaledCenter.x), static_cast<int>(scaledCenter.y), nullptr);
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
            
            TextOutA(memDC, 10, 10, "PDS-T1000-TSA24 - KLICK AUF KNOTEN FÜR ROUTE", 45);
            
            // Fahrzeug-Info
            int yOffset = 40;
            HFONT hInfoFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
                                         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                         DEFAULT_PITCH | FF_SWISS, "Arial");
            SelectObject(memDC, hInfoFont);
            
            for (size_t i = 0; i < current_autos.size(); i++) {
                const Auto& vehicle = current_autos[i];
                if (vehicle.isValid()) {
                    Point center = vehicle.getCenter();
                    float direction = vehicle.getDirection();
                    
                    char vehicleText[128];
                    snprintf(vehicleText, sizeof(vehicleText), "Auto %d: (%.0f, %.0f) %.0f°", 
                            vehicle.getId(), center.x, center.y, direction);
                    TextOutA(memDC, 10, yOffset, vehicleText, strlen(vehicleText));
                    yOffset += 20;
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
                // Transformiere Test-Fenster Koordinaten zu Raylib-Koordinaten
                RECT windowRect;
                GetClientRect(hwnd, &windowRect);
                int windowWidth = windowRect.right - windowRect.left;
                int windowHeight = windowRect.bottom - windowRect.top;
                
                int raylib_width = GetSystemMetrics(SM_CXSCREEN);
                int raylib_height = GetSystemMetrics(SM_CYSCREEN);
                
                float raylibX = (float)mouseX * raylib_width / windowWidth;
                float raylibY = (float)mouseY * raylib_height / windowHeight;
                
                Point clickPos(raylibX, raylibY);
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
                                    vehicleId, nearestNodeId, raylibX, raylibY);
                            MessageBoxA(hwnd, message, "Route geplant", MB_OK | MB_ICONINFORMATION);
                            break; // Nur für das erste Auto eine Route setzen
                        }
                    }
                } else {
                    char message[256];
                    snprintf(message, sizeof(message), "Kein Knoten gefunden bei (%.0f, %.0f)\nVersuche näher an einen Knoten zu klicken", 
                            raylibX, raylibY);
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
        
        // Fenster erstellen (maximiert)
        HWND hwnd = CreateWindowExA(
            WS_EX_TOPMOST,             // Immer im Vordergrund
            "TestWindow",
            "PDS-T1000-TSA24 - ZWEITES FENSTER",
            WS_OVERLAPPEDWINDOW,
            100,                       // X Position 
            100,                       // Y Position  
            1200,                      // Breite
            800,                       // Höhe
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
        
        ShowWindow(hwnd, SW_SHOWMAXIMIZED); // Maximiert starten
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
        
        // Konvertiere DetectedObjects zu Points mit Vollbild-Skalierung
        g_points.clear();
        
        // Hole Fenster-Dimensionen für Skalierung
        RECT windowRect;
        if (g_test_window_hwnd && GetClientRect(g_test_window_hwnd, &windowRect)) {
            int windowWidth = windowRect.right - windowRect.left;
            int windowHeight = windowRect.bottom - windowRect.top;
            
            for (const auto& obj : detected_objects) {
                // Normalisiere Koordinaten (0.0 bis 1.0)
                float norm_x = 0.0f;
                float norm_y = 0.0f;
                
                if (obj.crop_width > 0 && obj.crop_height > 0) {
                    norm_x = obj.coordinates.x / obj.crop_width;
                    norm_y = obj.coordinates.y / obj.crop_height;
                }
                
                // Verwende kalibrierte Transformation
                float window_x, window_y;
                transformCoordinates(norm_x, norm_y, windowWidth, windowHeight, window_x, window_y);
                
                if (obj.color == "Front") {
                    g_points.emplace_back(window_x, window_y, PointType::FRONT, obj.color);
                } else if (obj.color.find("Heck") == 0) {
                    g_points.emplace_back(window_x, window_y, PointType::IDENTIFICATION, obj.color);
                }
            }
        } else {
            // Fallback: Direkte Koordinaten ohne Skalierung
            for (const auto& obj : detected_objects) {
                if (obj.color == "Front") {
                    g_points.emplace_back(obj.coordinates.x, obj.coordinates.y, PointType::FRONT, obj.color);
                } else if (obj.color.find("Heck") == 0) {
                    g_points.emplace_back(obj.coordinates.x, obj.coordinates.y, PointType::IDENTIFICATION, obj.color);
                }
            }
        }
        
        // Auto-Erkennung durchführen
        detectVehiclesInTestWindow();
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

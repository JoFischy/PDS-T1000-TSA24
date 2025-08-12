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

// Alternative: Einfaches Windows API Fenster (ohne Raylib Includes hier)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

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
static float g_tolerance = 100.0f;

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

// Zeichne einen Punkt (wie in renderer.cpp)
void drawPointGDI(HDC hdc, const Point& point, bool isSelected) {
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
            static_cast<int>(point.x - radius), 
            static_cast<int>(point.y - radius),
            static_cast<int>(point.x + radius), 
            static_cast<int>(point.y + radius));
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
    
    // Label zeichnen
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    
    if (!point.color.empty()) {
        if (point.color == "Front") {
            TextOutA(hdc, static_cast<int>(point.x + 15), static_cast<int>(point.y - 15), "FRONT", 5);
        } else if (point.color.find("Heck") == 0) {
            std::string label = point.color;
            std::transform(label.begin(), label.end(), label.begin(), ::toupper);
            TextOutA(hdc, static_cast<int>(point.x + 15), static_cast<int>(point.y - 15), label.c_str(), label.length());
        }
    }
}

// Zeichne ein Auto (wie in renderer.cpp)
void drawAutoGDI(HDC hdc, const Auto& auto_) {
    if (!auto_.isValid()) return;

    Point idPoint = auto_.getIdentificationPoint();
    Point frontPoint = auto_.getFrontPoint();
    Point center = auto_.getCenter();

    // Grüne Linie zwischen den Punkten
    HPEN greenPen = CreatePen(PS_SOLID, 6, RGB(0, 255, 0));
    HGDIOBJ oldPen = SelectObject(hdc, greenPen);
    
    MoveToEx(hdc, static_cast<int>(idPoint.x), static_cast<int>(idPoint.y), nullptr);
    LineTo(hdc, static_cast<int>(frontPoint.x), static_cast<int>(frontPoint.y));
    
    SelectObject(hdc, oldPen);
    DeleteObject(greenPen);

    // Grüner Mittelpunkt
    HBRUSH greenBrush = CreateSolidBrush(RGB(0, 255, 0));
    HGDIOBJ oldBrush = SelectObject(hdc, greenBrush);
    
    int centerRadius = 8;
    Ellipse(hdc, 
            static_cast<int>(center.x - centerRadius), 
            static_cast<int>(center.y - centerRadius),
            static_cast<int>(center.x + centerRadius), 
            static_cast<int>(center.y + centerRadius));
    
    SelectObject(hdc, oldBrush);
    DeleteObject(greenBrush);

    // Oranger Richtungspfeil
    float dx = frontPoint.x - idPoint.x;
    float dy = frontPoint.y - idPoint.y;
    float length = sqrt(dx * dx + dy * dy);
    
    if (length > 0) {
        dx /= length;
        dy /= length;
        
        float arrowLength = 30.0f;
        float arrowEndX = idPoint.x + dx * arrowLength;
        float arrowEndY = idPoint.y + dy * arrowLength;
        
        HPEN orangePen = CreatePen(PS_SOLID, 4, RGB(255, 165, 0));
        SelectObject(hdc, orangePen);
        
        MoveToEx(hdc, static_cast<int>(idPoint.x), static_cast<int>(idPoint.y), nullptr);
        LineTo(hdc, static_cast<int>(arrowEndX), static_cast<int>(arrowEndY));
        
        // Pfeilspitze
        float headLength = 10.0f;
        float headAngle = 30.0f * M_PI / 180.0f;
        float dirRad = atan2(dy, dx);
        
        float head1X = arrowEndX - cos(dirRad - headAngle) * headLength;
        float head1Y = arrowEndY - sin(dirRad - headAngle) * headLength;
        float head2X = arrowEndX - cos(dirRad + headAngle) * headLength;
        float head2Y = arrowEndY - sin(dirRad + headAngle) * headLength;
        
        MoveToEx(hdc, static_cast<int>(arrowEndX), static_cast<int>(arrowEndY), nullptr);
        LineTo(hdc, static_cast<int>(head1X), static_cast<int>(head1Y));
        MoveToEx(hdc, static_cast<int>(arrowEndX), static_cast<int>(arrowEndY), nullptr);
        LineTo(hdc, static_cast<int>(head2X), static_cast<int>(head2Y));
        
        SelectObject(hdc, oldPen);
        DeleteObject(orangePen);
    }
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
            
            // Weißer Hintergrund (wie im Raylib-Fenster)
            FillRect(memDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
            
            // Hole aktuelle Daten (Thread-safe)
            std::vector<Point> current_points;
            std::vector<Auto> current_autos;
            {
                std::lock_guard<std::mutex> lock(g_data_mutex);
                current_points = g_points;
                current_autos = g_detected_autos;
            }
            
            // Zeichne Autos zuerst (damit sie hinter den Punkten erscheinen)
            for (const Auto& auto_ : current_autos) {
                drawAutoGDI(memDC, auto_);
            }
            
            // Zeichne alle Punkte
            for (const Point& point : current_points) {
                drawPointGDI(memDC, point, false);
            }
            
            // UI-Info links oben (minimal wie im Original)
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, RGB(0, 0, 0));
            HFONT hFont = CreateFontA(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
                                     CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                     DEFAULT_PITCH | FF_SWISS, "Arial");
            HGDIOBJ oldFont = SelectObject(memDC, hFont);
            
            TextOutA(memDC, 10, 10, "PDS-T1000-TSA24", 15);
            
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
        
        ShowWindow(hwnd, SW_SHOWMAXIMIZED); // Maximiert starten
        UpdateWindow(hwnd);
        
        // Timer für regelmäßige Updates (alle 200ms statt 100ms für weniger Flackern)
        SetTimer(hwnd, 1, 200, nullptr);
        
        std::cout << "Zweites Auto-Fenster maximiert auf Hauptmonitor erstellt!" << std::endl;
        
        // Message Loop
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
    }).detach();
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
                // Normalisiere Koordinaten (0.0 bis 1.0) und skaliere auf Fenstergröße
                float norm_x = 0.0f;
                float norm_y = 0.0f;
                
                if (obj.crop_width > 0 && obj.crop_height > 0) {
                    norm_x = obj.coordinates.x / obj.crop_width;
                    norm_y = obj.coordinates.y / obj.crop_height;
                }
                
                // Skaliere auf gesamte Fensterfläche
                float window_x = norm_x * windowWidth;
                float window_y = norm_y * windowHeight;
                
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
#endif

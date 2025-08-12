#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <sstream>
#include <iomanip>
#include "Vehicle.h"

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

// Globale Variablen für Koordinaten-Update
static std::vector<DetectedObject> g_detected_objects;
static std::mutex g_data_mutex;
static HWND g_test_window_hwnd = nullptr;

LRESULT CALLBACK TestWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Weißer Hintergrund
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
            
            // Schwarzer Text für bessere Lesbarkeit
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));
            
            // Titel
            HFONT hTitleFont = CreateFontA(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
                                         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                         DEFAULT_PITCH | FF_SWISS, "Arial");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hTitleFont);
            
            RECT titleRect = rect;
            titleRect.bottom = 60;
            DrawTextA(hdc, "LIVE AUTO-KOORDINATEN", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            // Koordinaten-Text
            HFONT hCoordFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
                                         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                         DEFAULT_PITCH | FF_SWISS, "Consolas");
            SelectObject(hdc, hCoordFont);
            
            // Hole aktuelle Koordinaten (Thread-safe)
            std::vector<DetectedObject> current_objects;
            {
                std::lock_guard<std::mutex> lock(g_data_mutex);
                current_objects = g_detected_objects;
            }
            
            // Koordinaten anzeigen
            int yPos = 80;
            const int lineHeight = 20;
            
            if (current_objects.empty()) {
                SetTextColor(hdc, RGB(128, 128, 128));
                TextOutA(hdc, 20, yPos, "Warte auf Koordinaten...", 24);
            } else {
                // Header
                SetTextColor(hdc, RGB(0, 0, 139)); // Dunkelblau
                std::string header = "ID   FARBE        X-KOORDINATE    Y-KOORDINATE    FLAECHE";
                TextOutA(hdc, 20, yPos, header.c_str(), header.length());
                yPos += lineHeight + 5;
                
                // Koordinaten-Daten
                SetTextColor(hdc, RGB(0, 0, 0)); // Schwarz
                for (const auto& obj : current_objects) {
                    std::ostringstream oss;
                    oss << std::setw(3) << obj.id << "  "
                        << std::setw(12) << std::left << obj.color << " "
                        << std::setw(12) << std::fixed << std::setprecision(2) << obj.coordinates.x << "  "
                        << std::setw(12) << std::fixed << std::setprecision(2) << obj.coordinates.y << "  "
                        << std::setw(8) << std::fixed << std::setprecision(1) << obj.area;
                    
                    std::string line = oss.str();
                    TextOutA(hdc, 20, yPos, line.c_str(), line.length());
                    yPos += lineHeight;
                    
                    // Verhindere Überlauf
                    if (yPos > rect.bottom - 40) break;
                }
                
                // Status-Info
                SetTextColor(hdc, RGB(0, 128, 0)); // Grün
                std::ostringstream statusOss;
                statusOss << "Objekte erkannt: " << current_objects.size() << " | Status: LIVE";
                std::string status = statusOss.str();
                TextOutA(hdc, 20, rect.bottom - 30, status.c_str(), status.length());
            }
            
            SelectObject(hdc, hOldFont);
            DeleteObject(hTitleFont);
            DeleteObject(hCoordFont);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER:
            // Automatische Aktualisierung alle 100ms
            InvalidateRect(hwnd, nullptr, TRUE);
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
            "LIVE AUTO-KOORDINATEN - RAYLIB PARALLEL",
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
        
        // Timer für regelmäßige Updates (alle 100ms)
        SetTimer(hwnd, 1, 100, nullptr);
        
        std::cout << "Live-Koordinaten-Fenster maximiert auf Hauptmonitor erstellt!" << std::endl;
        
        // Message Loop
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
    }).detach();
}

// Update-Funktion für Live-Koordinaten
void updateTestWindowCoordinates(const std::vector<DetectedObject>& detected_objects) {
    // Thread-safe Update der globalen Koordinaten
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        g_detected_objects = detected_objects;
    }
    
    // Fenster zur Neuzeichnung veranlassen (falls es existiert)
    if (g_test_window_hwnd != nullptr) {
        InvalidateRect(g_test_window_hwnd, nullptr, TRUE);
    }
}
#endif

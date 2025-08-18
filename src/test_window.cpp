#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <map>
#include <ctime>
#include "Vehicle.h"
#include "auto.h"
#include "point.h"
#include "path_system.h"
#include "vehicle_controller.h"
#include "serial_communication.h"

// Alternative: Einfaches Windows API Fenster (ohne Raylib Includes hier)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <commctrl.h>  // Für Trackbar-Controls

#pragma comment(lib, "comctl32.lib")  // Link Trackbar-Library

// Konstanten für Vollbild 1920x1200
static const int FULLSCREEN_WIDTH = 1920;
static const int FULLSCREEN_HEIGHT = 1200;

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
static float g_tolerance = 250.0f;
static HBITMAP g_backgroundBitmap = nullptr;

// Forward declarations
void updateTestWindowCoordinates(const std::vector<DetectedObject>& detected_objects);

// Persistente Fahrzeuge (bleiben auch bei kurzen Erkennungsaussetzern)
struct PersistentVehicle {
    Auto vehicle;
    std::chrono::steady_clock::time_point lastSeen;
    bool justUpdated;
    
    PersistentVehicle(const Auto& v) : vehicle(v), lastSeen(std::chrono::steady_clock::now()), justUpdated(true) {}
};
static std::vector<PersistentVehicle> g_persistent_vehicles;
static std::mutex g_persistent_mutex;
static const auto MAX_VEHICLE_AGE = std::chrono::seconds(3); // Fahrzeuge 3 Sekunden merken

// Globale Variablen für PathSystem und VehicleController
static const PathSystem* g_path_system = nullptr;
static const VehicleController* g_vehicle_controller = nullptr;

// Globale SerialCommunication Instanz
static SerialCommunication g_serial_comm;
static std::string g_selected_com_port = "";
static bool g_auto_send_commands = false;
static std::chrono::steady_clock::time_point g_last_esp_send = std::chrono::steady_clock::now();
static const int ESP_SEND_INTERVAL_MS = 10; // Alle 10ms ESP-Befehle senden (maximale Geschwindigkeit)

// ESP-Thread Variablen für völlige Unabhängigkeit von Kamera
static std::thread g_esp_thread;
static std::mutex g_esp_mutex;
static bool g_esp_thread_running = false;
static bool g_esp_thread_should_stop = false;

// Kurze Dreh-Pausen System (alle 50ms kurz anhalten beim Drehen)
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_last_command_times; // Letzte Befehlszeit pro Fahrzeug
static std::unordered_map<int, int> g_vehicle_step_state; // 0=Stop+Berechnen, 1=Ausführen
static const int STEP_DURATION_MS = 2000; // ULTRA-STABIL: 2 Sekunden für maximale Stabilität

// 🔍 VEREINFACHTES DEBUG-SYSTEM (weniger Spam) 🔍
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_camera_update_times; // Wann Kamera-Daten aktualisiert
static std::unordered_map<int, float> g_last_angles; // Letzte berechnete Winkel
static auto g_debug_start_time = std::chrono::steady_clock::now(); // System-Start für relative Zeiten
static int g_debug_counter = 0; // Reduziert Debug-Spam

// Globale Variablen für Fahrzeugauswahl und manuelles Auto
static int g_selected_vehicle_id = -1;
static Auto g_manual_vehicle;
static bool g_manual_vehicle_active = false;
static float g_manual_speed = 3.0f;

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

// Button-IDs für ESP Kommunikation
#define ID_BUTTON_CONNECT_ESP     2001
#define ID_BUTTON_DISCONNECT_ESP  2002
#define ID_BUTTON_SEND_COMMANDS   2003
#define ID_BUTTON_AUTO_SEND       2004
#define ID_COMBO_COM_PORTS        2005

// Button-IDs für serielle Kommunikation
#define ID_BUTTON_CONNECT_ESP     2001
#define ID_BUTTON_DISCONNECT_ESP  2002
#define ID_BUTTON_SEND_COMMANDS   2003
#define ID_BUTTON_AUTO_SEND       2004
#define ID_COMBO_COM_PORTS        2005

// Trackbar-Handles (nicht verwendet, aber für zukünftige Erweiterungen definiert)
// static HWND g_trackbar_x_scale = nullptr;
// static HWND g_trackbar_y_scale = nullptr;
// static HWND g_trackbar_x_offset = nullptr;
// static HWND g_trackbar_y_offset = nullptr;
// static HWND g_trackbar_x_curve = nullptr;
// static HWND g_trackbar_y_curve = nullptr;


// Einfache Funktion um aktuelle Auto-Position zu bekommen (für spätere Kamera-Integration)
Point getManualVehiclePosition() {
    if (g_manual_vehicle_active) {
        return g_manual_vehicle.getCenter();
    }
    return Point(0, 0);
}

// Funktion um manuelles Auto auf Kamera-Koordinaten zu setzen
void setManualVehicleFromCamera(float x, float y) {
    if (g_manual_vehicle_active) {
        Point newPos(x, y);
        g_manual_vehicle.setPosition(newPos);
        std::cout << "Vehicle position set from camera: (" << x << ", " << y << ")" << std::endl;
    }
}

// ESP-Thread Funktion - läuft völlig unabhängig von Kamera!
void espThreadFunction() {
    std::cout << "🚀 ESP-Thread gestartet - völlig unabhängig von Kamera!" << std::endl;
    g_esp_thread_running = true;
    
    while (!g_esp_thread_should_stop) {
        {
            std::lock_guard<std::mutex> lock(g_esp_mutex);
            
            if (g_auto_send_commands && !g_selected_com_port.empty()) {
                auto now = std::chrono::steady_clock::now();
                auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_esp_send).count();
                
                if (time_since_last >= ESP_SEND_INTERVAL_MS) {
                    // ESP-Kommunikation im separaten Thread - DAUERHAFTE Verbindung
                    static bool esp_connected = false;
                    
                    // Einmalig verbinden und Verbindung halten
                    if (!esp_connected) {
                        if (g_serial_comm.connect(g_selected_com_port, 115200)) {
                            esp_connected = true;
                            std::cout << "� ESP-Thread: Dauerhafte Verbindung hergestellt (" << g_selected_com_port << ")" << std::endl;
                        } else {
                            std::cout << "❌ ESP-Thread: Verbindung fehlgeschlagen (" << g_selected_com_port << ")" << std::endl;
                        }
                    }
                    
                    // Befehle senden (ohne disconnect!)
                    if (esp_connected) {
                        if (g_serial_comm.sendVehicleCommands()) {
                            // Nur alle 50 ESP-Messages (reduziert Spam)
                            static int esp_debug_counter = 0;
                            esp_debug_counter++;
                            if (esp_debug_counter % 50 == 0) {
                                std::cout << "📡 ESP-Thread: Befehle gesendet (" << g_selected_com_port << ")" << std::endl;
                            }
                        } else {
                            std::cout << "❌ ESP-Thread: Befehle fehlgeschlagen - Neuverbindung..." << std::endl;
                            // Bei Fehler: Verbindung zurücksetzen für Neuverbindung
                            esp_connected = false;
                            g_serial_comm.disconnect();
                        }
                        g_last_esp_send = now;
                    }
                }
            }
        }
        
        // Thread schläft 25ms - ultra-schnelle Reaktion für kurze präzise Drehungen
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    
    // Thread-Ende: Saubere Trennung der ESP-Verbindung
    g_serial_comm.disconnect();
    g_esp_thread_running = false;
    std::cout << "🛑 ESP-Thread beendet - Verbindung getrennt" << std::endl;
}

// Simuliere DetectedObject für Kompatibilität
void simulateDetectedObjectFromManualVehicle() {
    if (g_manual_vehicle_active) {
        std::vector<DetectedObject> simulated;
        DetectedObject obj;
        Point pos = g_manual_vehicle.getCenter();
        obj.coordinates = Point2D(pos.x, pos.y);  // Convert Point to Point2D
        obj.color = "Heck1";
        obj.crop_width = FULLSCREEN_WIDTH;
        obj.crop_height = FULLSCREEN_HEIGHT;
        simulated.push_back(obj);
        
        // Front-Point simulieren (10 Pixel nach vorn)
        DetectedObject frontObj;
        frontObj.coordinates = Point2D(pos.x, pos.y - 10);  // Convert Point to Point2D
        frontObj.color = "Front";
        frontObj.crop_width = FULLSCREEN_WIDTH;
        frontObj.crop_height = FULLSCREEN_HEIGHT;
        simulated.push_back(frontObj);
        
        updateTestWindowCoordinates(simulated);
    }
}

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
            std::cout << "🔧 DEBUG: Kalibrierungs-Fenster WM_CREATE wird ausgeführt..." << std::endl;
            
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
            y += 50;

            // Aktuelle Werte anzeigen
            CreateWindowA("STATIC", "--- Aktuelle Werte ---", WS_VISIBLE | WS_CHILD, 10, y, 200, 20, hwnd, nullptr, nullptr, nullptr);
            y += 30;
            CreateWindowA("STATIC", "X-Scale: 1.0, Y-Scale: 1.0", WS_VISIBLE | WS_CHILD, 10, y, 300, 20, hwnd, (HMENU)9001, nullptr, nullptr);
            y += 20;
            CreateWindowA("STATIC", "X-Offset: 0, Y-Offset: 0", WS_VISIBLE | WS_CHILD, 10, y, 300, 20, hwnd, (HMENU)9002, nullptr, nullptr);
            y += 40;

            // ESP Serielle Kommunikation Bereich
            CreateWindowA("STATIC", "--- ESP KOMUNIKATION ---", WS_VISIBLE | WS_CHILD, 10, y, 200, 20, hwnd, nullptr, nullptr, nullptr);
            y += 30;
            
            // COM-Port Auswahl
            CreateWindowA("STATIC", "COM-Port:", WS_VISIBLE | WS_CHILD, 10, y, 80, 20, hwnd, nullptr, nullptr, nullptr);
            HWND comboPort = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                          100, y, 100, 200, hwnd, (HMENU)ID_COMBO_COM_PORTS, nullptr, nullptr);
            
            // Verfügbare COM-Ports hinzufügen
            std::cout << "🔧 DEBUG: Suche verfügbare COM-Ports..." << std::endl;
            std::vector<std::string> ports = SerialCommunication::getAvailablePorts();
            std::cout << "🔧 DEBUG: " << ports.size() << " COM-Ports gefunden" << std::endl;
            
            int com5_index = -1;
            for (int i = 0; i < ports.size(); i++) {
                const auto& port = ports[i];
                std::cout << "🔧 DEBUG: Port " << i << ": " << port << std::endl;
                SendMessage(comboPort, CB_ADDSTRING, 0, (LPARAM)port.c_str());
                if (port == "COM5") {
                    com5_index = i;
                    std::cout << "🔧 DEBUG: COM5 gefunden bei Index " << i << std::endl;
                }
            }
            
            // ESP-Port bevorzugen (jeder verfügbare Port für ESP)
            std::cout << "🔧 DEBUG: COM-Port-Auswahl..." << std::endl;
            if (!ports.empty()) {
                std::cout << "🔧 DEBUG: Wähle ersten verfügbaren Port: " << ports[0] << std::endl;
                SendMessage(comboPort, CB_SETCURSEL, 0, 0);
                g_selected_com_port = ports[0];
                
                // ESP-Verbindung testen (jetzt auf separatem Port - kein Kamera-Konflikt)
                std::cout << "🔍 Teste ESP-Verfügbarkeit auf " << g_selected_com_port << "..." << std::endl;
                if (g_serial_comm.connect(g_selected_com_port, 115200)) {
                    std::cout << "✅ ESP verfügbar auf " << g_selected_com_port << " (separater Port - kein Kamera-Konflikt)" << std::endl;
                    
                    // Kurz trennen und wieder verbinden für dauerhafte Verbindung
                    g_serial_comm.disconnect();
                    
                    // Auto-Send aktivieren für regelmäßige Verbindung
                    g_auto_send_commands = true;
                    HWND checkbox = GetDlgItem(hwnd, ID_BUTTON_AUTO_SEND);
                    SendMessage(checkbox, BM_SETCHECK, BST_CHECKED, 0);
                    
                    // ESP-Thread starten für völlige Unabhängigkeit
                    if (!g_esp_thread_running) {
                        g_esp_thread_should_stop = false;
                        g_esp_thread = std::thread(espThreadFunction);
                        g_esp_thread.detach(); // Thread läuft völlig unabhängig
                    }
                    
                    std::cout << "🔄 Auto-Send aktiviert - ESP-Thread gestartet auf " << g_selected_com_port << std::endl;
                    
                    SetWindowTextA(GetDlgItem(hwnd, 9003), ("Status: ESP bereit (" + g_selected_com_port + " - separater Thread)").c_str());
                } else {
                    std::cout << "❌ ESP nicht verfügbar auf " << g_selected_com_port << std::endl;
                    SetWindowTextA(GetDlgItem(hwnd, 9003), ("Status: ESP nicht verfügbar (" + g_selected_com_port + ")").c_str());
                }
            } else {
                std::cout << "⚠️ Keine COM-Ports verfügbar!" << std::endl;
                SetWindowTextA(GetDlgItem(hwnd, 9003), "Status: Keine COM-Ports verfügbar");
            }
            y += 35;
            
            // ESP Verbindungs-Buttons
            CreateWindowA("BUTTON", "ESP Verbinden", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                         10, y, 100, 25, hwnd, (HMENU)ID_BUTTON_CONNECT_ESP, nullptr, nullptr);
            CreateWindowA("BUTTON", "Trennen", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                         120, y, 80, 25, hwnd, (HMENU)ID_BUTTON_DISCONNECT_ESP, nullptr, nullptr);
            y += 35;
            
            // Befehle senden
            CreateWindowA("BUTTON", "Befehle Senden", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                         10, y, 120, 25, hwnd, (HMENU)ID_BUTTON_SEND_COMMANDS, nullptr, nullptr);
            y += 35;
            
            // Auto-Send Toggle
            CreateWindowA("BUTTON", "Auto-Send", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                         10, y, 100, 20, hwnd, (HMENU)ID_BUTTON_AUTO_SEND, nullptr, nullptr);
            y += 30;
            
            // Status-Anzeige
            CreateWindowA("STATIC", "Status: Nicht verbunden", WS_VISIBLE | WS_CHILD,
                         10, y, 250, 20, hwnd, (HMENU)9003, nullptr, nullptr);

            return 0;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_BUTTON_CONNECT_ESP: {
                    if (!g_selected_com_port.empty()) {
                        if (g_serial_comm.connect(g_selected_com_port, 115200)) {
                            SetWindowTextA(GetDlgItem(hwnd, 9003), "Status: ESP verbunden!");
                            std::cout << "✅ ESP erfolgreich verbunden auf " << g_selected_com_port << std::endl;
                        } else {
                            SetWindowTextA(GetDlgItem(hwnd, 9003), "Status: Verbindung fehlgeschlagen!");
                            std::cout << "❌ ESP Verbindung fehlgeschlagen!" << std::endl;
                        }
                    }
                    break;
                }
                
                case ID_BUTTON_DISCONNECT_ESP: {
                    g_serial_comm.disconnect();
                    SetWindowTextA(GetDlgItem(hwnd, 9003), "Status: Nicht verbunden");
                    std::cout << "🔌 ESP Verbindung getrennt" << std::endl;
                    break;
                }
                
                case ID_BUTTON_SEND_COMMANDS: {
                    if (g_serial_comm.isConnectedToESP()) {
                        if (g_serial_comm.sendVehicleCommands()) {
                            SetWindowTextA(GetDlgItem(hwnd, 9003), "Status: Befehle gesendet!");
                            std::cout << "📡 Fahrzeugbefehle erfolgreich gesendet" << std::endl;
                        } else {
                            SetWindowTextA(GetDlgItem(hwnd, 9003), "Status: Senden fehlgeschlagen!");
                            std::cout << "❌ Senden der Fahrzeugbefehle fehlgeschlagen!" << std::endl;
                        }
                    } else {
                        MessageBoxA(hwnd, "ESP-Board ist nicht verbunden!", "Fehler", MB_OK | MB_ICONWARNING);
                    }
                    break;
                }
                
                case ID_BUTTON_AUTO_SEND: {
                    g_auto_send_commands = !g_auto_send_commands;
                    HWND checkbox = GetDlgItem(hwnd, ID_BUTTON_AUTO_SEND);
                    SendMessage(checkbox, BM_SETCHECK, g_auto_send_commands ? BST_CHECKED : BST_UNCHECKED, 0);
                    
                    if (g_auto_send_commands) {
                        std::cout << "🔄 Auto-Send aktiviert - ESP-Thread startet!" << std::endl;
                        
                        // ESP-Thread starten für völlige Unabhängigkeit
                        if (!g_esp_thread_running) {
                            g_esp_thread_should_stop = false;
                            g_esp_thread = std::thread(espThreadFunction);
                            g_esp_thread.detach(); // Thread läuft völlig unabhängig
                        }
                        
                        if (!g_serial_comm.isConnectedToESP()) {
                            MessageBoxA(hwnd, "Hinweis: ESP-Board ist nicht verbunden!\nBitte zuerst verbinden.", "Info", MB_OK | MB_ICONINFORMATION);
                        }
                    } else {
                        std::cout << "⏸️ Auto-Send deaktiviert - ESP-Thread stoppt!" << std::endl;
                        
                        // ESP-Thread stoppen
                        g_esp_thread_should_stop = true;
                        // Thread läuft detached und stoppt sich selbst
                    }
                    break;
                }
                
                case ID_COMBO_COM_PORTS: {
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        char portName[10];
                        SendMessage((HWND)lParam, CB_GETLBTEXT, index, (LPARAM)portName);
                        g_selected_com_port = std::string(portName);
                        std::cout << "📍 COM-Port ausgewählt: " << g_selected_com_port << std::endl;
                    }
                    break;
                }
            }
            return 0;
        }
        
        case WM_HSCROLL: {
            if (LOWORD(wParam) == TB_THUMBTRACK || LOWORD(wParam) == TB_ENDTRACK) {
                int trackbarId = GetDlgCtrlID((HWND)lParam);
                int position = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);

                switch (trackbarId) {
                    case ID_TRACKBAR_X_SCALE:
                        g_x_scale = trackbarToFloat(position, 0.5f, 2.0f);
                        printf("X-Scale geändert auf: %.3f\n", g_x_scale);
                        break;
                    case ID_TRACKBAR_Y_SCALE:
                        g_y_scale = trackbarToFloat(position, 0.5f, 2.0f);
                        printf("Y-Scale geändert auf: %.3f\n", g_y_scale);
                        break;
                    case ID_TRACKBAR_X_OFFSET:
                        g_x_offset = trackbarToFloat(position, -200.0f, 200.0f);
                        printf("X-Offset geändert auf: %.1f\n", g_x_offset);
                        break;
                    case ID_TRACKBAR_Y_OFFSET:
                        g_y_offset = trackbarToFloat(position, -200.0f, 200.0f);
                        printf("Y-Offset geändert auf: %.1f\n", g_y_offset);
                        break;
                    case ID_TRACKBAR_X_CURVE:
                        g_x_curve = trackbarToFloat(position, -0.5f, 0.5f);
                        printf("X-Curve geändert auf: %.3f\n", g_x_curve);
                        break;
                    case ID_TRACKBAR_Y_CURVE:
                        g_y_curve = trackbarToFloat(position, -0.5f, 0.5f);
                        printf("Y-Curve geändert auf: %.3f\n", g_y_curve);
                        break;
                }

                // Hauptfenster neu zeichnen
                if (g_test_window_hwnd) {
                    InvalidateRect(g_test_window_hwnd, nullptr, FALSE);
                }

                // Kalibrierungs-Werte-Anzeige aktualisieren
                char buffer1[100], buffer2[100];
                sprintf(buffer1, "X-Scale: %.2f, Y-Scale: %.2f", g_x_scale, g_y_scale);
                sprintf(buffer2, "X-Offset: %.0f, Y-Offset: %.0f", g_x_offset, g_y_offset);
                SetWindowTextA(GetDlgItem(hwnd, 9001), buffer1);
                SetWindowTextA(GetDlgItem(hwnd, 9002), buffer2);
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
// Monitor-Enum-Struktur für Monitor-3-Erkennung
struct MonitorInfo {
    int count = 0;
    RECT monitor3Rect = {0, 0, 0, 0};
    bool found = false;
};

// Callback-Funktion für Monitor-Enumeration
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MonitorInfo* info = reinterpret_cast<MonitorInfo*>(dwData);
    info->count++;
    
    // Monitor 3 ist der dritte Monitor (Index 2, aber count ist bereits incrementiert)
    if (info->count == 3) {
        info->monitor3Rect = *lprcMonitor;
        info->found = true;
        return FALSE; // Stoppe Enumeration
    }
    return TRUE; // Weiter mit nächstem Monitor
}

// Funktion zum Ermitteln der Monitor-3-Position
RECT getMonitor3Position() {
    MonitorInfo info;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&info));
    
    if (info.found) {
        std::cout << "Monitor 3 gefunden: " << info.monitor3Rect.left << ", " 
                  << info.monitor3Rect.top << " - " << info.monitor3Rect.right 
                  << ", " << info.monitor3Rect.bottom << std::endl;
        return info.monitor3Rect;
    } else {
        std::cout << "Monitor 3 nicht gefunden, verwende Standard-Position" << std::endl;
        // Fallback: Standard-Position
        RECT fallback = {100, 100, 450, 400};
        return fallback;
    }
}

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

    // Monitor 3 Position ermitteln
    RECT monitor3Rect = getMonitor3Position();
    
    // Fenster auf Monitor 3 positionieren (obere linke Ecke)
    int windowWidth = 350;
    int windowHeight = 500;  // Größer für ESP-Kommunikations-Controls
    int posX = monitor3Rect.left + 10; // 10px vom linken Rand
    int posY = monitor3Rect.top + 10;  // 10px vom oberen Rand

    // Kalibrierungs-Fenster ÜBER dem Vollbild erstellen (TOPMOST)
    g_calibration_window = CreateWindowExA(
        WS_EX_TOPMOST,  // TOPMOST - bleibt IMMER im Vordergrund
        "CalibrationWindow",
        "Koordinaten-Kalibrierung (verschiebbar)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE | WS_THICKFRAME,
        posX, posY, windowWidth, windowHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (g_calibration_window) {
        // Stelle sicher, dass das Fenster immer oben bleibt
        SetWindowPos(g_calibration_window, HWND_TOPMOST, 0, 0, 0, 0, 
                     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        ShowWindow(g_calibration_window, SW_SHOW);
        UpdateWindow(g_calibration_window);
        std::cout << "Kalibrierungs-Fenster TOPMOST erstellt - immer im Vordergrund!" << std::endl;
        std::cout << "Position: " << posX << ", " << posY << " - Sie können es frei verschieben!" << std::endl;
    } else {
        std::cout << "Fehler beim Erstellen des Kalibrierungs-Fensters!" << std::endl;
    }
}

// Direkte Crop-zu-Vollbild Koordinaten-Transformation
void transformCropToFullscreen(float crop_x, float crop_y, float crop_width, float crop_height, float& fullscreen_x, float& fullscreen_y) {
    // Die crop_x und crop_y sind bereits Pixel-Koordinaten im Crop-Bereich
    // Diese werden direkt auf 1920x1200 Vollbild skaliert

    if (crop_width > 0 && crop_height > 0 && crop_x >= 0 && crop_y >= 0) {
        // Normalisiere auf 0.0 bis 1.0 basierend auf Crop-Größe
        float norm_x = crop_x / crop_width;
        float norm_y = crop_y / crop_height;

        // Skaliere direkt auf 1920x1200 Vollbild mit Kalibrierung
        fullscreen_x = (norm_x * FULLSCREEN_WIDTH * g_x_scale) + g_x_offset;
        fullscreen_y = (norm_y * FULLSCREEN_HEIGHT * g_y_scale) + g_y_offset;

        // Kurvenkorrektur anwenden
        float center_x = FULLSCREEN_WIDTH / 2.0f;
        float center_y = FULLSCREEN_HEIGHT / 2.0f;
        float x_diff = fullscreen_x - center_x;
        float y_diff = fullscreen_y - center_y;

        fullscreen_x += x_diff * g_x_curve;
        fullscreen_y += y_diff * g_y_curve;

        // Gültigkeitsprüfung - verhindere 0/0 Sprünge
        if (fullscreen_x < 0 || fullscreen_x > FULLSCREEN_WIDTH || 
            fullscreen_y < 0 || fullscreen_y > FULLSCREEN_HEIGHT) {
            printf("WARNING: Invalid transformed coordinates (%.2f, %.2f) - using fallback\n", fullscreen_x, fullscreen_y);
            fullscreen_x = center_x;  // Fallback zur Mitte statt 0/0
            fullscreen_y = center_y;
        }
    } else {
        printf("WARNING: Invalid input coordinates: crop_x=%.2f, crop_y=%.2f, crop_size=(%.2f, %.2f)\n", 
               crop_x, crop_y, crop_width, crop_height);
        // Fallback zur Mitte statt 0/0
        fullscreen_x = FULLSCREEN_WIDTH / 2.0f;
        fullscreen_y = FULLSCREEN_HEIGHT / 2.0f;
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

// Zeichne ein Punkt (bereits in Vollbild-Koordinaten)
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

// Verwaltung persistenter Fahrzeuge
void updatePersistentVehicles(const std::vector<Auto>& newDetections) {
    std::lock_guard<std::mutex> lock(g_persistent_mutex);
    auto now = std::chrono::steady_clock::now();
    
    // Markiere alle als nicht aktualisiert
    for (auto& persistent : g_persistent_vehicles) {
        persistent.justUpdated = false;
    }
    
    // Aktualisiere oder füge neue Fahrzeuge hinzu
    for (const auto& newVehicle : newDetections) {
        if (!newVehicle.isValid()) continue;
        
        bool found = false;
        for (auto& persistent : g_persistent_vehicles) {
            // Fahrzeug gefunden (gleiche ID oder sehr nahe Position)
            if (persistent.vehicle.getId() == newVehicle.getId() || 
                persistent.vehicle.getCenter().distanceTo(newVehicle.getCenter()) < 200.0f) {
                
                // Sanfte Position Update
                Point oldPos = persistent.vehicle.getCenter();
                Point newPos = newVehicle.getCenter();
                
                // Interpoliere Position um sanfte Bewegung zu erreichen
                float alpha = 0.7f; // Gewichtung für neue Position
                Point smoothPos;
                smoothPos.x = oldPos.x * (1.0f - alpha) + newPos.x * alpha;
                smoothPos.y = oldPos.y * (1.0f - alpha) + newPos.y * alpha;
                
                // Update des Fahrzeugs mit sanfter Position
                persistent.vehicle = newVehicle;
                persistent.vehicle.setPosition(smoothPos);
                persistent.lastSeen = now;
                persistent.justUpdated = true;
                found = true;
                break;
            }
        }
        
        // Neues Fahrzeug hinzufügen
        if (!found) {
            g_persistent_vehicles.emplace_back(newVehicle);
        }
    }
    
    // Entferne zu alte Fahrzeuge
    g_persistent_vehicles.erase(
        std::remove_if(g_persistent_vehicles.begin(), g_persistent_vehicles.end(),
            [now](const PersistentVehicle& pv) {
                return (now - pv.lastSeen) > MAX_VEHICLE_AGE;
            }),
        g_persistent_vehicles.end()
    );
}

std::vector<Auto> getPersistentVehicles() {
    std::lock_guard<std::mutex> lock(g_persistent_mutex);
    std::vector<Auto> result;
    
    for (const auto& persistent : g_persistent_vehicles) {
        result.push_back(persistent.vehicle);
    }
    
    return result;
}

// JSON-Commands für Fahrzeugsteuerung schreiben
// Globale Variable für JSON-Anzeige
std::string g_json_display_text = "";

void updateVehicleCommands() {
    if (!g_vehicle_controller || !g_path_system) {
        return;
    }

    // PRÄZISIONS-IMPULSE: Kurze Drehbefehle mit Pausen für Neuberechnung
    static auto last_update = std::chrono::steady_clock::now();
    static std::map<int, std::chrono::steady_clock::time_point> vehicle_last_turn_time; // Letzte Drehzeit pro Fahrzeug
    static std::map<int, int> vehicle_turn_state; // 0=warten, 1=drehen_links, 2=drehen_rechts, 3=fahren
    
    auto now = std::chrono::steady_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();
    
    // Nur alle 10ms für sehr schnelle Richtungsänderungen und präzise Kontrolle
    if (time_diff < 10) {
        return;
    }
    last_update = now;

    // JSON-Datei erstellen/überschreiben
    std::ofstream jsonFile("vehicle_commands.json");
    jsonFile << "{\n  \"vehicles\": [\n";

    bool firstVehicle = true;
    std::vector<Auto> current_autos = getPersistentVehicles();
    
    // Erstelle Display-Text für Live-Anzeige
    g_json_display_text = "=== PRÄZISIONS-IMPULSE SYSTEM ===\n";
    g_json_display_text += "Autos: " + std::to_string(current_autos.size()) + "\n\n";

    // PRÄZISE IMPULSE BEFEHLSLOGIK - KURZE DREH-IMPULSE!
    for (const Auto& auto_ : current_autos) {
        if (!auto_.isValid()) continue;

        if (!firstVehicle) jsonFile << ",\n";
        firstVehicle = false;

        int vehicle_id = auto_.getId();
        int command = 0; // Default: Stopp
        
        // Initialisiere Fahrzeug-Status falls nötig
        if (vehicle_last_turn_time.find(vehicle_id) == vehicle_last_turn_time.end()) {
            vehicle_last_turn_time[vehicle_id] = now;
            vehicle_turn_state[vehicle_id] = 0; // Warten
        }
        
        // Finde das entsprechende Vehicle im Controller
        const auto& vehicles = g_vehicle_controller->getAllVehicles();
        
        for (const auto& [controllerId, controllerVehicle] : vehicles) {
            if (controllerVehicle.vehicleId == vehicle_id) {
                Point currentPos = auto_.getCenter();
                
                // Prüfe ob Fahrzeug eine aktive Route hat
                if (!controllerVehicle.currentNodePath.empty() && 
                    controllerVehicle.currentNodeIndex < controllerVehicle.currentNodePath.size()) {

                    int nextNodeId = controllerVehicle.currentNodePath[controllerVehicle.currentNodeIndex];
                    const PathNode* targetNode = g_path_system->getNode(nextNodeId);
                    
                    if (targetNode) {
                        Point targetPos = targetNode->position;
                        
                        float routeDx = targetPos.x - currentPos.x;
                        float routeDy = targetPos.y - currentPos.y;
                        float distanceToTarget = sqrt(routeDx * routeDx + routeDy * routeDy);

                        if (distanceToTarget > 20.0f) {
                            // Berechne gewünschte Richtung zur Route
                            float targetAngle = atan2(-routeDy, routeDx) * 180.0f / M_PI;
                            if (targetAngle < 0) targetAngle += 360.0f;

                            // Hole aktuellen Winkel
                            Point frontPoint = auto_.getFrontPoint();
                            float currentDx = frontPoint.x - currentPos.x;
                            float currentDy = frontPoint.y - currentPos.y;
                            float currentAngle = atan2(-currentDy, currentDx) * 180.0f / M_PI;
                            if (currentAngle < 0) currentAngle += 360.0f;

                            // Berechne Winkeldifferenz (-180 bis +180)
                            float angleDiff = targetAngle - currentAngle;
                            if (angleDiff > 180.0f) angleDiff -= 360.0f;
                            if (angleDiff < -180.0f) angleDiff += 360.0f;

                            // Zeit seit letztem Drehbefehl
                            auto time_since_turn = std::chrono::duration_cast<std::chrono::milliseconds>(now - vehicle_last_turn_time[vehicle_id]).count();

                            // PRÄZISIONS-NAVIGATION: Kleine Toleranz (4°) mit Impulsen
                            if (abs(angleDiff) > 4.0f) {  // 4° Toleranz für hohe Präzision
                                
                                // Impulse-System: 50ms drehen, dann 100ms warten für Neuberechnung
                                if (vehicle_turn_state[vehicle_id] == 0 && time_since_turn > 100) {
                                    // Starte neuen Drehimpuls - KORRIGIERTE RICHTUNG!
                                    command = (angleDiff > 0) ? 3 : 4; // RECHTS=3, LINKS=4 (KORRIGIERT!)
                                    vehicle_turn_state[vehicle_id] = (angleDiff > 0) ? 2 : 1;
                                    vehicle_last_turn_time[vehicle_id] = now;
                                    
                                    std::cout << "🔄 DREH-IMPULS START: ID=" << vehicle_id 
                                             << " DIFF=" << angleDiff << "° CMD=" << command << "\n";
                                } else if (vehicle_turn_state[vehicle_id] > 0 && time_since_turn < 50) {
                                    // Drehimpuls läuft noch (50ms) - KORRIGIERTE RICHTUNG!
                                    command = (vehicle_turn_state[vehicle_id] == 2) ? 3 : 4;
                                    
                                    std::cout << "🔄 DREH-IMPULS AKTIV: ID=" << vehicle_id 
                                             << " Zeit=" << time_since_turn << "ms CMD=" << command << "\n";
                                } else if (vehicle_turn_state[vehicle_id] > 0 && time_since_turn >= 50) {
                                    // Drehimpuls beendet, warte für Neuberechnung
                                    command = 0; // STOPP
                                    vehicle_turn_state[vehicle_id] = 0; // Zurück zu Warten
                                    
                                    std::cout << "⏸️ DREH-PAUSE: ID=" << vehicle_id 
                                             << " DIFF=" << angleDiff << "° STOPP für Neuberechnung\n";
                                } else {
                                    // Noch in Wartezeit
                                    command = 0; // STOPP
                                }
                                
                            } else {
                                // Winkel ist gut genug - VORWÄRTS-IMPULS-SYSTEM!
                                // Impulse-System: 50ms vorwärts, dann 100ms warten
                                if (vehicle_turn_state[vehicle_id] == 0 && time_since_turn > 100) {
                                    // Starte Vorwärts-Impuls
                                    command = 1; // Vorwärts
                                    vehicle_turn_state[vehicle_id] = 3; // Fahrmodus
                                    vehicle_last_turn_time[vehicle_id] = now;
                                    
                                    std::cout << "🎯 VORWÄRTS-IMPULS START: ID=" << vehicle_id 
                                             << " DIFF=" << angleDiff << "° FAHRE!\n";
                                } else if (vehicle_turn_state[vehicle_id] == 3 && time_since_turn < 50) {
                                    // Vorwärts-Impuls läuft noch (50ms)
                                    command = 1; // Vorwärts
                                    
                                    std::cout << "🎯 VORWÄRTS-IMPULS AKTIV: ID=" << vehicle_id 
                                             << " Zeit=" << time_since_turn << "ms\n";
                                } else if (vehicle_turn_state[vehicle_id] == 3 && time_since_turn >= 50) {
                                    // Vorwärts-Impuls beendet, kurze Pause
                                    command = 0; // STOPP
                                    vehicle_turn_state[vehicle_id] = 0; // Zurück zu Warten
                                    
                                    std::cout << "⏸️ VORWÄRTS-PAUSE: ID=" << vehicle_id 
                                             << " DIFF=" << angleDiff << "° STOPP für Neuberechnung\n";
                                } else {
                                    // Noch in Wartezeit
                                    command = 0; // STOPP
                                }
                            }
                            
                            // DEBUG: Detaillierte Ausgabe
                            std::cout << "📐 PRÄZISION [" << time_diff << "ms] ID=" << vehicle_id 
                                     << " ANG=" << currentAngle << "° -> " << targetAngle 
                                     << "° DIFF=" << angleDiff << "° STATE=" << vehicle_turn_state[vehicle_id] 
                                     << " CMD=" << command << "\n";
                        }
                    }
                }
                break;
            }
        }
        
        // JSON schreiben
        jsonFile << "    {\n";
        jsonFile << "      \"id\": " << vehicle_id << ",\n";
        jsonFile << "      \"command\": " << command << "\n";
        jsonFile << "    }";
        
        // Display-Text aktualisieren
        g_json_display_text += "ID: " + std::to_string(vehicle_id) + " -> " + std::to_string(command) + "\n";
    }

    jsonFile << "\n  ]\n}";
    jsonFile.close();
    
    std::cout << "📡 PRÄZISION UPDATE: JSON geschrieben (200ms Intervall)\n";
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

    // Auto-Mittelpunkt - unterschiedliche Farbe für ausgewähltes Auto
    COLORREF autoColor = RGB(0, 255, 0);      // Grün für normale Autos
    COLORREF borderColor = RGB(0, 180, 0);

    if (auto_.getId() == g_selected_vehicle_id) {
        autoColor = RGB(255, 0, 255);         // Magenta für ausgewähltes Auto
        borderColor = RGB(200, 0, 200);
    }

    HBRUSH autoBrush = CreateSolidBrush(autoColor);
    HGDIOBJ oldBrush = SelectObject(hdc, autoBrush);
    HPEN autoPen = CreatePen(PS_SOLID, 4, borderColor);  // Dicker Rahmen
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

    // === FAHRZEUG-NUMMER ANZEIGEN ===
    if (auto_.getId() > 0) {
        // Zeige die Fahrzeug-ID als Text über dem Auto
        std::string vehicleText = "ID:" + std::to_string(auto_.getId());
        
        // Schwarzer Hintergrund für bessere Lesbarkeit
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, RGB(0, 0, 0));
        SetTextColor(hdc, RGB(255, 255, 255)); // Weiße Schrift
        
        // Text über dem Auto platzieren
        TextOut(hdc, static_cast<int>(fullscreenCenter.x - 15), 
                     static_cast<int>(fullscreenCenter.y - 30), 
                     vehicleText.c_str(), vehicleText.length());
        
        // Transparenten Hintergrund wieder herstellen
        SetBkMode(hdc, TRANSPARENT);
    }

    // === NEUER ZIELPFEIL: Zeigt zum finalen Ziel ===
    if (g_vehicle_controller && g_path_system) {
        // Finde das entsprechende Vehicle im Controller
        const auto& vehicles = g_vehicle_controller->getAllVehicles();
        
        for (const auto& vehicle : vehicles) {
            // Versuche zuerst direkte ID-Zuordnung
            bool isMatchingVehicle = (vehicle.second.vehicleId == auto_.getId());
            
            // Falls ID=0, versuche Position-basiertes Matching
            if (!isMatchingVehicle && auto_.getId() == 0) {
                Point autoPos = auto_.getCenter();
                float distance = sqrt(pow(vehicle.second.position.x - autoPos.x, 2) + pow(vehicle.second.position.y - autoPos.y, 2));
                if (distance < 100.0f) { // Wenn Auto näher als 100 Pixel zu Controller-Vehicle
                    isMatchingVehicle = true;
                }
            }
            
            if (isMatchingVehicle && vehicle.second.targetNodeId != -1) {
                // Hole den finalen Zielknoten
                const PathNode* targetNode = g_path_system->getNode(vehicle.second.targetNodeId);
                if (targetNode) {
                    Point targetPos = mapToFullscreenCoordinates(targetNode->position.x, targetNode->position.y);
                    
                    // Berechne Vektor zum finalen Ziel
                    float targetDx = targetPos.x - fullscreenCenter.x;
                    float targetDy = targetPos.y - fullscreenCenter.y;
                    float targetLength = sqrt(targetDx * targetDx + targetDy * targetDy);
                    
                    if (targetLength > 0) {
                        targetDx /= targetLength;
                        targetDy /= targetLength;
                        
                        // Zeichne ZIELPFEIL (blau, länger als Richtungspfeil)
                        HPEN targetPen = CreatePen(PS_SOLID, 3, RGB(0, 150, 255)); // Cyan-Blau
                        HGDIOBJ oldTargetPen = SelectObject(hdc, targetPen);
                        
                        float targetArrowLength = 40.0f; // Länger als Richtungspfeil
                        float targetEndX = fullscreenCenter.x + targetDx * targetArrowLength;
                        float targetEndY = fullscreenCenter.y + targetDy * targetArrowLength;
                        
                        // Hauptlinie des Zielpfeils
                        MoveToEx(hdc, static_cast<int>(fullscreenCenter.x), static_cast<int>(fullscreenCenter.y), nullptr);
                        LineTo(hdc, static_cast<int>(targetEndX), static_cast<int>(targetEndY));
                        
                        // Zielpfeil-Spitze (größer)
                        float targetHeadLength = 12.0f;
                        float targetHeadAngle = 0.4f;
                        
                        float targetLeftX = targetEndX - (targetDx * cos(targetHeadAngle) - targetDy * sin(targetHeadAngle)) * targetHeadLength;
                        float targetLeftY = targetEndY - (targetDx * sin(targetHeadAngle) + targetDy * cos(targetHeadAngle)) * targetHeadLength;
                        float targetRightX = targetEndX - (targetDx * cos(-targetHeadAngle) - targetDy * sin(-targetHeadAngle)) * targetHeadLength;
                        float targetRightY = targetEndY - (targetDx * sin(-targetHeadAngle) + targetDy * cos(-targetHeadAngle)) * targetHeadLength;
                        
                        MoveToEx(hdc, static_cast<int>(targetEndX), static_cast<int>(targetEndY), nullptr);
                        LineTo(hdc, static_cast<int>(targetLeftX), static_cast<int>(targetLeftY));
                        MoveToEx(hdc, static_cast<int>(targetEndX), static_cast<int>(targetEndY), nullptr);
                        LineTo(hdc, static_cast<int>(targetRightX), static_cast<int>(targetRightY));
                        
                        SelectObject(hdc, oldTargetPen);
                        DeleteObject(targetPen);
                    }
                }
                
                // === NEUER ROUTENPFEIL: Zeigt zum nächsten Wegpunkt in der Route ===
                if (!vehicle.second.currentNodePath.empty() && vehicle.second.currentNodeIndex < vehicle.second.currentNodePath.size()) {
                    // Hole den nächsten Knoten aus der Route
                    int currentNodeId = vehicle.second.currentNodePath[vehicle.second.currentNodeIndex];
                    const PathNode* targetNode = g_path_system->getNode(currentNodeId);
                    
                    if (targetNode) {
                        // In node-based navigation, we navigate directly to the target node
                        Point vehiclePos = mapToFullscreenCoordinates(vehicle.second.position.x, vehicle.second.position.y);
                        Point targetPos = mapToFullscreenCoordinates(targetNode->position.x, targetNode->position.y);
                        
                        // Berechne Vektor zum nächsten Wegpunkt
                        float routeDx = targetPos.x - fullscreenCenter.x;
                        float routeDy = targetPos.y - fullscreenCenter.y;
                        float routeLength = sqrt(routeDx * routeDx + routeDy * routeDy);
                        
                        if (routeLength > 10.0f) { // Nur zeichnen wenn Wegpunkt nicht zu nah
                            routeDx /= routeLength;
                            routeDy /= routeLength;
                            
                            // Zeichne ROUTENPFEIL (grün, mittlere Länge)
                            HPEN routePen = CreatePen(PS_SOLID, 4, RGB(0, 255, 0)); // Helles Grün
                            HGDIOBJ oldRoutePen = SelectObject(hdc, routePen);
                            
                            float routeArrowLength = 32.0f; // Zwischen Richtungs- und Zielpfeil
                            float routeEndX = fullscreenCenter.x + routeDx * routeArrowLength;
                            float routeEndY = fullscreenCenter.y + routeDy * routeArrowLength;
                            
                            // Hauptlinie des Routenpfeils
                            MoveToEx(hdc, static_cast<int>(fullscreenCenter.x), static_cast<int>(fullscreenCenter.y), nullptr);
                            LineTo(hdc, static_cast<int>(routeEndX), static_cast<int>(routeEndY));
                            
                            // Routenpfeil-Spitze
                            float routeHeadLength = 10.0f;
                            float routeHeadAngle = 0.45f;
                            
                            float routeLeftX = routeEndX - (routeDx * cos(routeHeadAngle) - routeDy * sin(routeHeadAngle)) * routeHeadLength;
                            float routeLeftY = routeEndY - (routeDx * sin(routeHeadAngle) + routeDy * cos(routeHeadAngle)) * routeHeadLength;
                            float routeRightX = routeEndX - (routeDx * cos(-routeHeadAngle) - routeDy * sin(-routeHeadAngle)) * routeHeadLength;
                            float routeRightY = routeEndY - (routeDx * sin(-routeHeadAngle) + routeDy * cos(-routeHeadAngle)) * routeHeadLength;
                            
                            MoveToEx(hdc, static_cast<int>(routeEndX), static_cast<int>(routeEndY), nullptr);
                            LineTo(hdc, static_cast<int>(routeLeftX), static_cast<int>(routeLeftY));
                            MoveToEx(hdc, static_cast<int>(routeEndX), static_cast<int>(routeEndY), nullptr);
                            LineTo(hdc, static_cast<int>(routeRightX), static_cast<int>(routeRightY));
                            
                            SelectObject(hdc, oldRoutePen);
                            DeleteObject(routePen);
                        }
                    }
                }
            }
        }
    }
}

// Draw the node network
void drawNodeNetworkGDI(HDC hdc, const PathSystem& pathSystem) {
    // Zeichne alle Segmente (Pfade)
    for (const auto& segment : pathSystem.getSegments()) {
        drawPathSegmentGDI(hdc, segment, pathSystem);
    }

    // Zeichne alle Knoten
    for (const auto& node : pathSystem.getNodes()) {
        drawPathNodeGDI(hdc, node);
    }
}

// Zeichne Fahrzeugroute (einzelne Version ohne Duplikate)
void drawVehicleRouteGDI(HDC hdc, const Auto& vehicle, const PathSystem& pathSystem) {
    if (!g_vehicle_controller) return;

    // Finde das entsprechende Vehicle im Controller
    const auto& vehicles = g_vehicle_controller->getAllVehicles();
    for (const auto& controllerVehicle : vehicles) {
        if (controllerVehicle.second.vehicleId == vehicle.getId()) {
            if (controllerVehicle.second.currentNodePath.empty()) continue; // Updated to new logic

            // Spezielle Hervorhebung für ausgewähltes Fahrzeug
            COLORREF routeColor;
            int routeWidth;

            if (vehicle.getId() == g_selected_vehicle_id) {
                routeColor = RGB(255, 50, 255);  // Helles Magenta für ausgewähltes Fahrzeug
                routeWidth = 12;                 // Extra dick für bessere Sichtbarkeit
            } else {
                // Standard-Routenfarben (nur schwach sichtbar wenn nicht ausgewählt)
                COLORREF routeColors[] = {
                    RGB(100, 200, 100),   // Schwaches Grün
                    RGB(200, 200, 100),   // Schwaches Gelb  
                    RGB(200, 150, 100),   // Schwaches Orange
                    RGB(150, 100, 150)    // Schwaches Lila
                };
                routeColor = routeColors[vehicle.getId() % 4];
                routeWidth = 4; // Dünner für unausgewählte Fahrzeuge
            }

            HPEN routePen = CreatePen(PS_SOLID, routeWidth, routeColor);
            HGDIOBJ oldPen = SelectObject(hdc, routePen);

            // ROUTE DRAWING - Neue Knoten-basierte Logik
            Point currentPos = mapToFullscreenCoordinates(vehicle.getCenter().x, vehicle.getCenter().y);
            
            // Zeichne Route durch alle Knoten im currentNodePath
            for (size_t i = controllerVehicle.second.currentNodeIndex; i < controllerVehicle.second.currentNodePath.size(); i++) {
                int nodeId = controllerVehicle.second.currentNodePath[i];
                const PathNode* node = pathSystem.getNode(nodeId);
                
                if (node) {
                    Point nodePos = mapToFullscreenCoordinates(node->position.x, node->position.y);
                    
                    // Aktueller Wegpunkt extra hervorheben bei ausgewähltem Fahrzeug
                    if (i == controllerVehicle.second.currentNodeIndex && vehicle.getId() == g_selected_vehicle_id) {
                        SelectObject(hdc, oldPen);
                        DeleteObject(routePen);
                        routePen = CreatePen(PS_SOLID, 16, RGB(255, 100, 255)); // Extra dick und hell
                        SelectObject(hdc, routePen);
                    }
                    
                    // Zeichne Linie von aktueller Position zum Knoten
                    MoveToEx(hdc, static_cast<int>(currentPos.x), 
                            static_cast<int>(currentPos.y), nullptr);
                    LineTo(hdc, static_cast<int>(nodePos.x), 
                           static_cast<int>(nodePos.y));
                    
                    // Zurück zur normalen Linienbreite nach aktuellem Wegpunkt
                    if (i == controllerVehicle.second.currentNodeIndex && vehicle.getId() == g_selected_vehicle_id) {
                        SelectObject(hdc, oldPen);
                        DeleteObject(routePen);
                        routePen = CreatePen(PS_SOLID, routeWidth, routeColor);
                        SelectObject(hdc, routePen);
                    }
                    
                    // Aktualisiere Position für nächste Iteration
                    currentPos = nodePos;
                }
            }

            SelectObject(hdc, oldPen);
            DeleteObject(routePen);

            // Zeichne Zielknoten extra prominent für ausgewähltes Fahrzeug
            if (controllerVehicle.second.targetNodeId != -1) {
                const PathNode* targetNode = pathSystem.getNode(controllerVehicle.second.targetNodeId);
                if (targetNode) {
                    Point targetPos = mapToFullscreenCoordinates(targetNode->position.x, targetNode->position.y);

                    // Größerer und auffälligerer Zielknoten für ausgewähltes Fahrzeug
                    int targetRadius = (vehicle.getId() == g_selected_vehicle_id) ? 30 : 15;
                    COLORREF targetColor = (vehicle.getId() == g_selected_vehicle_id) ? 
                                          RGB(255, 50, 255) : routeColor;

                    HBRUSH targetBrush = CreateSolidBrush(targetColor);
                    HPEN targetPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 255)); // Weißer Rand
                    HGDIOBJ oldBrush = SelectObject(hdc, targetBrush);
                    HGDIOBJ oldTargetPen = SelectObject(hdc, targetPen);

                    Ellipse(hdc, 
                            static_cast<int>(targetPos.x - targetRadius), 
                            static_cast<int>(targetPos.y - targetRadius),
                            static_cast<int>(targetPos.x + targetRadius), 
                            static_cast<int>(targetPos.y + targetRadius));

                    SelectObject(hdc, oldBrush);
                    SelectObject(hdc, oldTargetPen);
                    DeleteObject(targetBrush);
                    DeleteObject(targetPen);

                    // Label für Zielknoten bei ausgewähltem Fahrzeug
                    if (vehicle.getId() == g_selected_vehicle_id) {
                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, RGB(255, 255, 255));
                        char targetLabel[32];
                        snprintf(targetLabel, sizeof(targetLabel), "ZIEL: %d", controllerVehicle.second.targetNodeId);
                        TextOutA(hdc, static_cast<int>(targetPos.x + 35), static_cast<int>(targetPos.y - 10), 
                                targetLabel, strlen(targetLabel));
                    }
                }
            }

            // RICHTUNGSPFEIL: Zeichne Pfeil vom Fahrzeug zum nächsten Wegpunkt
            Point nextTargetPos;
            bool hasTarget = false;
            
            // NEUE LOGIK: Knoten-basierte Navigation
            if (!controllerVehicle.second.currentNodePath.empty() && controllerVehicle.second.currentNodeIndex < controllerVehicle.second.currentNodePath.size()) {
                // Zeige zum nächsten Knoten in der Route
                int nextNodeId = controllerVehicle.second.currentNodePath[controllerVehicle.second.currentNodeIndex];
                const PathNode* nextNode = pathSystem.getNode(nextNodeId);
                
                if (nextNode) {
                    nextTargetPos = mapToFullscreenCoordinates(nextNode->position.x, nextNode->position.y);
                    hasTarget = true;
                    
                }
            } else if (controllerVehicle.second.targetNodeId != -1) {
                // Keine aktive Route, aber Ziel gesetzt: zeige zum finalen Zielknoten
                const PathNode* finalTargetNode = pathSystem.getNode(controllerVehicle.second.targetNodeId);
                if (finalTargetNode) {
                    nextTargetPos = mapToFullscreenCoordinates(finalTargetNode->position.x, finalTargetNode->position.y);
                    Point vehiclePos = vehicle.getCenter();
                    float distanceToFinalTarget = vehiclePos.distanceTo(nextTargetPos);
                    
                    // Nur zeigen wenn noch nicht am finalen Ziel angekommen
                    if (distanceToFinalTarget > 40.0f) {
                        hasTarget = true;
                    }
                }
            }
            
            if (hasTarget) {
                Point vehiclePos = vehicle.getCenter();
                
                // Berechne Richtungsvektor
                float dx = nextTargetPos.x - vehiclePos.x;
                float dy = nextTargetPos.y - vehiclePos.y;
                float length = sqrt(dx*dx + dy*dy);
                
                if (length > 10.0f) { // Nur zeichnen wenn nächster Punkt weit genug weg ist
                    // Normalisiere Richtung
                    dx /= length;
                    dy /= length;
                    
                    // Pfeil-Parameter
                    float arrowLength = 80.0f;  // Länge des Pfeils
                    float arrowHeadSize = 20.0f; // Größe der Pfeilspitze
                    
                    // Pfeil-Ende-Position
                    Point arrowEnd;
                    arrowEnd.x = vehiclePos.x + dx * arrowLength;
                    arrowEnd.y = vehiclePos.y + dy * arrowLength;
                    
                    // Zeichne Pfeil-Linie (dick und gut sichtbar)
                    HPEN arrowPen = CreatePen(PS_SOLID, 6, RGB(0, 255, 255)); // Cyan für gute Sichtbarkeit
                    HGDIOBJ oldArrowPen = SelectObject(hdc, arrowPen);
                            
                    MoveToEx(hdc, static_cast<int>(vehiclePos.x), static_cast<int>(vehiclePos.y), nullptr);
                    LineTo(hdc, static_cast<int>(arrowEnd.x), static_cast<int>(arrowEnd.y));
                    
                    // Zeichne Pfeilspitze
                    float headAngle = 0.5f; // Winkel der Pfeilspitze
                    Point head1, head2;
                    
                    head1.x = arrowEnd.x - dx * arrowHeadSize * cos(headAngle) + dy * arrowHeadSize * sin(headAngle);
                    head1.y = arrowEnd.y - dy * arrowHeadSize * cos(headAngle) - dx * arrowHeadSize * sin(headAngle);
                    
                    head2.x = arrowEnd.x - dx * arrowHeadSize * cos(headAngle) - dy * arrowHeadSize * sin(headAngle);
                    head2.y = arrowEnd.y - dy * arrowHeadSize * cos(headAngle) + dx * arrowHeadSize * sin(headAngle);
                    
                    // Zeichne Pfeilspitze-Linien
                    MoveToEx(hdc, static_cast<int>(arrowEnd.x), static_cast<int>(arrowEnd.y), nullptr);
                    LineTo(hdc, static_cast<int>(head1.x), static_cast<int>(head1.y));
                    
                    MoveToEx(hdc, static_cast<int>(arrowEnd.x), static_cast<int>(arrowEnd.y), nullptr);
                    LineTo(hdc, static_cast<int>(head2.x), static_cast<int>(head2.y));
                    
                    SelectObject(hdc, oldArrowPen);
                    DeleteObject(arrowPen);
                }
            }
            break;
        }
    }
}

// Draw the vehicle
void drawVehicleGDI(HDC hdc, const Auto& vehicle) {
    if (!vehicle.isValid()) return;

    Point center = vehicle.getCenter();
    Point frontPoint = vehicle.getFrontPoint();

    Point fullscreenCenter = center;
    Point fullscreenFront = frontPoint;

    float dx = fullscreenFront.x - fullscreenCenter.x;
    float dy = fullscreenFront.y - fullscreenCenter.y;
    float length = sqrt(dx * dx + dy * dy);

    if (length > 0) {
        dx /= length;
        dy /= length;
    }

    COLORREF autoColor = RGB(0, 255, 0);      // Green for normal cars
    COLORREF borderColor = RGB(0, 180, 0);

    if (vehicle.getId() == g_selected_vehicle_id) {
        autoColor = RGB(255, 0, 255);         // Magenta for selected car
        borderColor = RGB(200, 0, 200);
    }

    HBRUSH autoBrush = CreateSolidBrush(autoColor);
    HGDIOBJ oldBrush = SelectObject(hdc, autoBrush);
    HPEN autoPen = CreatePen(PS_SOLID, 4, borderColor);
    HGDIOBJ oldPen = SelectObject(hdc, autoPen);

    int radius = 12;
    Ellipse(hdc,
            static_cast<int>(fullscreenCenter.x - radius),
            static_cast<int>(fullscreenCenter.y - radius),
            static_cast<int>(fullscreenCenter.x + radius),
            static_cast<int>(fullscreenCenter.y + radius));

    SelectObject(hdc, oldBrush);
    DeleteObject(autoBrush);

    HPEN arrowPen = CreatePen(PS_SOLID, 4, RGB(255, 100, 0));
    SelectObject(hdc, arrowPen);

    float arrowLength = 25.0f;
    float arrowEndX = fullscreenCenter.x + dx * arrowLength;
    float arrowEndY = fullscreenCenter.y + dy * arrowLength;

    MoveToEx(hdc, static_cast<int>(fullscreenCenter.x), static_cast<int>(fullscreenCenter.y), nullptr);
    LineTo(hdc, static_cast<int>(arrowEndX), static_cast<int>(arrowEndY));

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

// Paint handler for the test window
void OnTestWindowPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // Zeichne Hintergrundbild falls vorhanden
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    // Verbessertes Double-Buffering
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT);
    HGDIOBJ oldMemBitmap = SelectObject(memDC, memBitmap);
    
    if (g_backgroundBitmap) {
        HDC bgDC = CreateCompatibleDC(hdc);
        HGDIOBJ oldBgBitmap = SelectObject(bgDC, g_backgroundBitmap);
        
        // Skaliere Hintergrundbild auf Vollbild (1920x1200)
        StretchBlt(memDC, 0, 0, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT,
                   bgDC, 0, 0, 
                   rect.right, rect.bottom, SRCCOPY);
        
        SelectObject(bgDC, oldBgBitmap);
        DeleteDC(bgDC);
    } else {
        // Fallback: Schwarzer Hintergrund
        HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
        RECT fullRect = {0, 0, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT};
        FillRect(memDC, &fullRect, blackBrush);
        DeleteObject(blackBrush);
    }

    // === LIVE JSON ANZEIGE oben links ===
    SetTextColor(memDC, RGB(0, 255, 0)); // Grüner Text
    SetBkMode(memDC, TRANSPARENT);
    
    // Teile den Display-Text in Zeilen auf
    std::istringstream iss(g_json_display_text);
    std::string line;
    int lineY = 10;
    
    while (std::getline(iss, line)) {
        TextOutA(memDC, 10, lineY, line.c_str(), line.length());
        lineY += 20; // 20 Pixel Abstand zwischen Zeilen
    }
    
    // Zeichne manuelles Test-Auto falls aktiv
    if (g_manual_vehicle_active) {
        drawAutoGDI(memDC, g_manual_vehicle);
        
        // Status-Informationen anzeigen
        SetTextColor(memDC, RGB(255, 255, 255));
        SetBkMode(memDC, TRANSPARENT);
        
        std::string statusText = "Manual Control Active - Use Arrow Keys to move";
        TextOutA(memDC, 10, 10, statusText.c_str(), statusText.length());
        
        char posText[100];
        Point pos = g_manual_vehicle.getCenter();
        sprintf(posText, "Position: (%.1f, %.1f) - Speed: %.1f - SPACE to change speed", 
                pos.x, pos.y, g_manual_speed);
        TextOutA(memDC, 10, 30, posText, strlen(posText));
        
        std::string instructionText = "UP/DOWN: Forward/Backward, LEFT/RIGHT: Turn, ESC: Exit";
        TextOutA(memDC, 10, 50, instructionText.c_str(), instructionText.length());
    }
    
    if (g_path_system) {
        // Zeichne Knotennetz
        drawNodeNetworkGDI(memDC, *g_path_system);

        // Verwende persistente Fahrzeuge für stabilere Anzeige
        std::vector<Auto> current_autos = getPersistentVehicles();

        // Zeichne nur gültige Fahrzeuge
        for (const auto& vehicle : current_autos) {
            if (vehicle.isValid()) {
                drawVehicleRouteGDI(memDC, vehicle, *g_path_system);
                drawVehicleGDI(memDC, vehicle);
            }
        }

        // Zeige ausgewähltes Fahrzeug an
        if (g_selected_vehicle_id != -1) {
            // Zeichne Text für ausgewähltes Fahrzeug (rechts von JSON)
            SetTextColor(memDC, RGB(255, 255, 0)); // Gelb für Auswahl
            SetBkMode(memDC, TRANSPARENT);

            std::string selectedText = ">>> FAHRZEUG " + std::to_string(g_selected_vehicle_id) + " AUSGEWAEHLT <<<";
            TextOutA(memDC, 300, 10, selectedText.c_str(), selectedText.length());

            std::string instructionText = "Klicken Sie auf einen Knoten um Ziel zu setzen";
            TextOutA(memDC, 300, 30, instructionText.c_str(), instructionText.length());
        } else {
            SetTextColor(memDC, RGB(200, 200, 200)); // Grau für Anleitung
            SetBkMode(memDC, TRANSPARENT);

            std::string instructionText = "Klicken Sie auf ein Fahrzeug um es auszuwaehlen";
            TextOutA(memDC, 300, 10, instructionText.c_str(), instructionText.length());
            
            std::string helpText = "Mehrere Fahrzeuge nacheinander anklickbar";
            TextOutA(memDC, 300, 30, helpText.c_str(), helpText.length());
        }
    }
    
    // Kopiere das fertige Bild in einem Rutsch auf den Bildschirm
    BitBlt(hdc, 0, 0, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT, memDC, 0, 0, SRCCOPY);
    
    // Aufräumen
    SelectObject(memDC, oldMemBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK TestWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
            OnTestWindowPaint(hwnd);
            return 0;

        case WM_TIMER:
            // Kontinuierlich Vehicle Commands aktualisieren
            updateVehicleCommands();
            
            // ESP-Kommunikation läuft jetzt in separatem Thread - NICHT HIER!
            
            // Nur bei Timer-Events neu zeichnen mit weniger Flackern
            InvalidateRect(hwnd, nullptr, FALSE);  // FALSE = kein Hintergrund löschen
            return 0;

        case WM_LBUTTONDOWN: {
            int mouseX = LOWORD(lParam);
            int mouseY = HIWORD(lParam);
            Point clickPos(static_cast<float>(mouseX), static_cast<float>(mouseY));

            // Verwende persistente Fahrzeuge für Klick-Erkennung
            std::vector<Auto> current_autos = getPersistentVehicles();

            if (g_selected_vehicle_id == -1) {
                // Kein Fahrzeug ausgewählt - versuche ein Fahrzeug zu selektieren
                for (const auto& vehicle : current_autos) {
                    Point vehiclePos = vehicle.getCenter();
                    float distance = clickPos.distanceTo(vehiclePos);

                    if (distance <= 50.0f) { // 50 Pixel Toleranz
                        g_selected_vehicle_id = vehicle.getId();
                        std::cout << "Fahrzeug " << g_selected_vehicle_id << " ausgewählt" << std::endl;
                        InvalidateRect(hwnd, NULL, FALSE); // Weniger Flackern
                        break;
                    }
                }
            } else {
                // Fahrzeug ist ausgewählt - versuche Zielknoten zu setzen
                if (g_path_system && g_vehicle_controller) {
                    int nearestNodeId = g_path_system->findNearestNode(clickPos, 80.0f);
                    if (nearestNodeId != -1) {
                        // Ziel für das ausgewählte Fahrzeug setzen
                        const_cast<VehicleController*>(g_vehicle_controller)->setVehicleTargetNode(g_selected_vehicle_id, nearestNodeId);
                        std::cout << "Fahrzeug " << g_selected_vehicle_id << " Ziel gesetzt auf Knoten " << nearestNodeId << std::endl;
                        
                        // Aktualisiere Fahrzeug-Befehle für die nächste Iteration
                        updateVehicleCommands(); 

                        // Fahrzeugauswahl aufheben damit nächstes Fahrzeug gewählt werden kann
                        g_selected_vehicle_id = -1;
                        std::cout << "Bereit für nächste Fahrzeugauswahl" << std::endl;

                        InvalidateRect(hwnd, NULL, FALSE); // Weniger Flackern
                    } else {
                        char message[256];
                        snprintf(message, sizeof(message), "Kein Knoten gefunden bei (%.0f, %.0f)\nVersuche näher an einen Knoten zu klicken",
                                (double)mouseX, (double)mouseY);
                        MessageBoxA(hwnd, message, "Kein Ziel gefunden", MB_OK | MB_ICONWARNING);
                    }
                }
            }
            return 0;
        }

        case WM_RBUTTONDOWN: {
            // Rechtsklick: Fahrzeugauswahl aufheben
            if (g_selected_vehicle_id != -1) {
                g_selected_vehicle_id = -1;
                std::cout << "Fahrzeugauswahl aufgehoben" << std::endl;
                InvalidateRect(hwnd, NULL, TRUE); // Neu zeichnen
            }
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            }
            
            // Pfeiltasten für manuelle Steuerung
            if (g_manual_vehicle_active) {
                Point currentPos = g_manual_vehicle.getCenter();
                Point newPos = currentPos;
                bool moved = false;
                
                switch (wParam) {
                    case VK_UP:    // Vorwärts
                        newPos.y -= g_manual_speed;
                        moved = true;
                        std::cout << "Moving forward to (" << newPos.x << ", " << newPos.y << ")" << std::endl;
                        break;
                    case VK_DOWN:  // Rückwärts
                        newPos.y += g_manual_speed;
                        moved = true;
                        std::cout << "Moving backward to (" << newPos.x << ", " << newPos.y << ")" << std::endl;
                        break;
                    case VK_LEFT:  // Links drehen/bewegen
                        newPos.x -= g_manual_speed;
                        moved = true;
                        std::cout << "Moving left to (" << newPos.x << ", " << newPos.y << ")" << std::endl;
                        break;
                    case VK_RIGHT: // Rechts drehen/bewegen
                        newPos.x += g_manual_speed;
                        moved = true;
                        std::cout << "Moving right to (" << newPos.x << ", " << newPos.y << ")" << std::endl;
                        break;
                    case VK_SPACE: // Geschwindigkeit ändern
                        g_manual_speed = (g_manual_speed == 3.0f) ? 6.0f : 3.0f;
                        std::cout << "Speed changed to " << g_manual_speed << std::endl;
                        break;
                }
                
                if (moved) {
                    // Grenzen prüfen
                    if (newPos.x < 50) newPos.x = 50;
                    if (newPos.x > FULLSCREEN_WIDTH - 50) newPos.x = FULLSCREEN_WIDTH - 50;
                    if (newPos.y < 50) newPos.y = 50;
                    if (newPos.y > FULLSCREEN_HEIGHT - 50) newPos.y = FULLSCREEN_HEIGHT - 50;
                    
                    g_manual_vehicle.setPosition(newPos);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void createWindowsAPITestWindow() {
    std::thread([]() {
        // Initialisiere manuelles Test-Auto in der Mitte
        Point startPos(FULLSCREEN_WIDTH / 2.0f, FULLSCREEN_HEIGHT / 2.0f);
        Point frontPos(startPos.x + 20.0f, startPos.y); // 20 Pixel nach rechts
        g_manual_vehicle = Auto(startPos, frontPos);  // Use existing constructor with 2 Points
        g_manual_vehicle_active = true;
        
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

        // Timer für regelmäßige Updates (alle 100ms für stabilere Darstellung)
        SetTimer(hwnd, 1, 100, nullptr);

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
// Öffentliche Funktion für kalibrierte Koordinaten-Transformation
void getCalibratedTransform(float crop_x, float crop_y, float crop_width, float crop_height, float& fullscreen_x, float& fullscreen_y) {
    transformCropToFullscreen(crop_x, crop_y, crop_width, crop_height, fullscreen_x, fullscreen_y);
}

void updateTestWindowCoordinates(const std::vector<DetectedObject>& detected_objects) {
    // Thread-safe Update der globalen Koordinaten
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        g_detected_objects = detected_objects;

        // Konvertiere DetectedObjects zu Points mit Crop-zu-Vollbild Transformation
        g_points.clear();

        // Convert detected objects to points with fullscreen coordinates
        for (const auto& obj : detected_objects) {
            // Überspringe ungültige Objekte sofort
            if (obj.crop_width <= 0 || obj.crop_height <= 0 || 
                obj.coordinates.x < 0 || obj.coordinates.y < 0) {
                continue;
            }

            float fullscreen_x, fullscreen_y;
            transformCropToFullscreen(obj.coordinates.x, obj.coordinates.y, 
                                    obj.crop_width, obj.crop_height, 
                                    fullscreen_x, fullscreen_y);

            // Erweiterte Gültigkeitsprüfung
            bool isValid = (fullscreen_x >= 50 && fullscreen_x <= FULLSCREEN_WIDTH - 50 && 
                           fullscreen_y >= 50 && fullscreen_y <= FULLSCREEN_HEIGHT - 50);

            if (isValid) {
                // Create point with correct type and color
                if (obj.color == "Front") {
                    g_points.emplace_back(fullscreen_x, fullscreen_y, PointType::FRONT, obj.color);
                } else if (obj.color.find("Heck") == 0) {
                    g_points.emplace_back(fullscreen_x, fullscreen_y, PointType::IDENTIFICATION, obj.color);
                }
            }
        }

        // Detect vehicles from the updated points
        detectVehiclesInTestWindow();
    }

    // Fenster zur Neuzeichnung veranlassen (falls es existiert)
    // SOFORTIGE UPDATES für Live-Darstellung
    if (g_test_window_hwnd != nullptr) {
        InvalidateRect(g_test_window_hwnd, nullptr, FALSE); // FALSE = kein Hintergrund löschen
        UpdateWindow(g_test_window_hwnd); // Sofortiges Update ohne Timer-Wartezeit
    }
}

// Update-Funktion für erkannte Autos von der Hauptanwendung
void updateTestWindowVehicles(const std::vector<Auto>& vehicles) {
    std::lock_guard<std::mutex> lock(g_data_mutex);
    g_detected_autos = vehicles;
    
    // Aktualisiere persistente Fahrzeuge
    updatePersistentVehicles(vehicles);

    // Fenster zur Neuzeichnung veranlassen
    if (g_test_window_hwnd != nullptr) {
        InvalidateRect(g_test_window_hwnd, nullptr, FALSE);
    }
}
#endif
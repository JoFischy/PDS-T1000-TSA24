#include "../include/BeamerProjection.h"
#include <iostream>

BeamerProjection::BeamerProjection() 
    : show_warnings(false) {
}

BeamerProjection::~BeamerProjection() {
    cleanup();
}

bool BeamerProjection::initialize(const char* title) {
    try {
        // Das Hauptfenster ist bereits in main.cpp initialisiert
        // Wir verwenden das bestehende Raylib-Fenster
        
        std::cout << "Beamer-Projektion initialisiert für Raylib Hauptfenster" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Fehler bei Beamer-Projektion Initialisierung: " << e.what() << std::endl;
        return false;
    }
}

void BeamerProjection::cleanup() {
    // Das Hauptfenster wird in main.cpp geschlossen
    std::cout << "Beamer-Projektion bereinigt" << std::endl;
}

void BeamerProjection::update(const std::vector<VehicleDetectionData>& fleet_data) {
    // Aktualisiere Fahrzeug-Daten für Warnungen
    vehicle_data = fleet_data;
    
    // Hier könnte später Logik für automatische Warnung-Erkennung hinzugefügt werden
    // z.B. wenn Fahrzeuge zu nah beieinander sind oder gefährliche Positionen haben
}

void BeamerProjection::draw() {
    BeginDrawing();
    
    // Weißer Hintergrund
    ClearBackground(WHITE);
    
    // Schwarzer Rand um die Projektion
    draw_border();
    
    // Zeichne alle erkannten Fahrzeuge als Rechtecke
    draw_vehicles();
    
    // Falls Warnungen aktiviert sind, zeige sie an
    if (show_warnings) {
        draw_warning_overlay();
    }
    
    EndDrawing();
}

void BeamerProjection::draw_border() {
    // Schwarzer Rahmen um den gesamten Bildschirm
    Color border_color = BLACK;
    
    // Hole aktuelle Fenstergröße
    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();
    
    // Oberer Rand
    DrawRectangle(0, 0, screen_width, BORDER_WIDTH, border_color);
    
    // Unterer Rand
    DrawRectangle(0, screen_height - BORDER_WIDTH, screen_width, BORDER_WIDTH, border_color);
    
    // Linker Rand
    DrawRectangle(0, 0, BORDER_WIDTH, screen_height, border_color);
    
    // Rechter Rand
    DrawRectangle(screen_width - BORDER_WIDTH, 0, BORDER_WIDTH, screen_height, border_color);
}

void BeamerProjection::draw_warning_overlay() {
    // Placeholder für zukünftige Warnungen
    // Hier können später Warnungs-Symbole, Texte oder Bereiche gezeichnet werden
    
    // Hole aktuelle Fenstergröße
    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();
    
    // Beispiel: Einfacher Warnungstext in der Mitte
    const char* warning_text = "WARNUNG AKTIVIERT";
    int text_width = MeasureText(warning_text, 40);
    
    DrawText(warning_text, 
             (screen_width - text_width) / 2, 
             screen_height / 2, 
             40, 
             RED);
}

void BeamerProjection::draw_vehicle_warnings() {
    // Placeholder für fahrzeugspezifische Warnungen
    // Hier können später basierend auf vehicle_data spezifische Warnbereiche
    // auf dem Boden projiziert werden
    
    for (size_t i = 0; i < vehicle_data.size(); i++) {
        const auto& vehicle = vehicle_data[i];
        
        if (vehicle.detected) {
            // Hier könnte später eine Projektion der Fahrzeugposition
            // auf dem Boden-Koordinatensystem erfolgen
        }
    }
}

void BeamerProjection::set_warning_mode(bool enabled) {
    show_warnings = enabled;
    
    if (enabled) {
        std::cout << "Beamer-Warnungen aktiviert" << std::endl;
    } else {
        std::cout << "Beamer-Warnungen deaktiviert" << std::endl;
    }
}

bool BeamerProjection::is_window_ready() const {
    return IsWindowReady();
}

bool BeamerProjection::should_close() const {
    return WindowShouldClose();
}

void BeamerProjection::draw_vehicles() {
    // Zeichne alle erkannten Fahrzeuge als farbige Rechtecke
    for (size_t i = 0; i < vehicle_data.size(); i++) {
        const auto& vehicle = vehicle_data[i];
        
        if (vehicle.detected) {
            // Konvertiere Fahrzeugposition zu Bildschirmkoordinaten
            int screen_width = GetScreenWidth();
            int screen_height = GetScreenHeight();
            
            // KORRIGIERTE Koordinaten-Transformation:
            // Fahrzeug-Koordinaten: X = rechts/links, Y = vor/zurück
            // Raylib-Koordinaten: X = rechts/links, Y = unten/oben
            
            float scale_x = (screen_width - 2 * BORDER_WIDTH) / 1000.0f;
            float scale_y = (screen_height - 2 * BORDER_WIDTH) / 1000.0f;
            
            // KORREKTE Zuordnung: 
            // - Fahrzeug fährt nach rechts (X+) -> Raylib nach rechts (X+) ✓
            // - Fahrzeug fährt nach unten (Y+) -> Raylib nach unten (Y+) - NICHT invertieren!
            int x = (int)(vehicle.position.x * scale_x) + BORDER_WIDTH;
            int y = (int)(vehicle.position.y * scale_y) + BORDER_WIDTH;  // Y NICHT invertiert!
            
            // Fahrzeuggröße (20x30 Pixel Rechteck)
            int vehicle_width = 20;
            int vehicle_height = 30;
            
            // Farbe basierend auf Heckfarbe
            Color vehicle_color = BLACK;  // Default
            if (vehicle.rear_color == "Blau") vehicle_color = BLUE;
            else if (vehicle.rear_color == "Grün") vehicle_color = GREEN;
            else if (vehicle.rear_color == "Gelb") vehicle_color = YELLOW;
            else if (vehicle.rear_color == "Lila") vehicle_color = PURPLE;
            else if (vehicle.rear_color == "Rot") vehicle_color = RED;
            
            // Zeichne Fahrzeug als gefülltes Rechteck
            DrawRectangle(x - vehicle_width/2, y - vehicle_height/2, 
                         vehicle_width, vehicle_height, vehicle_color);
            
            // Schwarzer Rahmen um das Fahrzeug
            DrawRectangleLines(x - vehicle_width/2, y - vehicle_height/2, 
                              vehicle_width, vehicle_height, BLACK);
            
            // Fahrzeug-ID als Text
            std::string vehicle_id = std::to_string(i + 1);
            DrawText(vehicle_id.c_str(), x - 5, y - 5, 12, WHITE);
            
            // KORRIGIERTER Richtungsindikator (Winkel angepasst)
            // Fahrzeug-Winkel: 0° = nach oben, 90° = nach rechts
            // Raylib-Winkel: 0° = nach rechts, 90° = nach unten
            // Daher: Fahrzeug-Winkel - 90° für Raylib
            float corrected_angle = (vehicle.angle - 90.0f) * DEG2RAD;
            int arrow_length = 15;
            int arrow_x = x + (int)(cos(corrected_angle) * arrow_length);
            int arrow_y = y + (int)(sin(corrected_angle) * arrow_length);
            
            DrawLine(x, y, arrow_x, arrow_y, BLACK);
            DrawCircle(arrow_x, arrow_y, 3, BLACK);
        }
    }
    
    // Koordinatensystem-Legende
    int legend_x = GetScreenWidth() - 250;
    int legend_y = 20;
    DrawText("FAHRZEUG-POSITIONEN", legend_x, legend_y, 16, BLACK);
    DrawText("Quadrat = Fahrzeug", legend_x, legend_y + 20, 12, DARKGRAY);
    DrawText("Linie = Fahrtrichtung", legend_x, legend_y + 35, 12, DARKGRAY);
    DrawText("Zahl = Fahrzeug-ID", legend_x, legend_y + 50, 12, DARKGRAY);
    DrawText("Unten = Vorwärts", legend_x, legend_y + 70, 10, DARKGRAY);
    DrawText("Rechts = Rechts", legend_x, legend_y + 85, 10, DARKGRAY);
}

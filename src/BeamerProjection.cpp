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
    
    // Weißer Hintergrund für optimale Kamera-Erkennung
    ClearBackground(WHITE);
    
    // Lila Kamera-Markierungsrahmen
    draw_camera_frame();
    
    // Zeichne alle erkannten Fahrzeuge als Rechtecke
    draw_vehicles();
    
    // Falls Warnungen aktiviert sind, zeige sie an
    if (show_warnings) {
        draw_warning_overlay();
    }
    
    EndDrawing();
}

void BeamerProjection::draw_camera_frame() {
    // INTENSIVE ROTE Eckpunkte als Kamera-Markierung - sehr kontrastreich auf weißem Hintergrund
    Color corner_color = RED;  // Rot - maximaler Kontrast zu weiß, optimal für Kamera-Erkennung
    
    // Hole aktuelle Fenstergröße
    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();
    
    // Größere, intensivere Eckpunkte für bessere Erkennung
    int corner_radius = 35;  // 70px Durchmesser - sehr gut sichtbar
    int margin = 15;         // Geringerer Abstand vom Bildschirmrand
    
    // Zeichne 4 MAGENTA Eckpunkte
    // Oben Links
    DrawCircle(margin + corner_radius, margin + corner_radius, (float)corner_radius, corner_color);
    
    // Oben Rechts  
    DrawCircle(screen_width - margin - corner_radius, margin + corner_radius, (float)corner_radius, corner_color);
    
    // Unten Links
    DrawCircle(margin + corner_radius, screen_height - margin - corner_radius, (float)corner_radius, corner_color);
    
    // Unten Rechts
    DrawCircle(screen_width - margin - corner_radius, screen_height - margin - corner_radius, (float)corner_radius, corner_color);
    
    // Informationstext in der Mitte oben
    const char* info_text = "KAMERA-ERKENNUNGSBEREICH - 4 ROTE Eckpunkte";
    int text_width = MeasureText(info_text, 16);
    DrawText(info_text, 
             (screen_width - text_width) / 2, 
             15, 
             16, 
             BLACK);  // Schwarzer Text auf weißem Hintergrund
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
            
            // Verfügbarer Bereich zwischen den Eckpunkten (85px Abstand von jeder Seite)
            const int corner_margin = 85;  // Platz für größere Eckpunkte + Abstand
            float scale_x = (screen_width - 2 * corner_margin) / 1000.0f;
            float scale_y = (screen_height - 2 * corner_margin) / 1000.0f;
            
            // KORREKTE Zuordnung: 
            // - Fahrzeug fährt nach rechts (X+) -> Raylib nach rechts (X+) ✓
            // - Fahrzeug fährt nach unten (Y+) -> Raylib nach unten (Y+) - NICHT invertieren!
            int x = (int)(vehicle.position.x * scale_x) + corner_margin;
            int y = (int)(vehicle.position.y * scale_y) + corner_margin;  // Y NICHT invertiert!
            
            // Fahrzeuggröße (schwarzes Rechteck mit Markierungspunkten)
            int vehicle_width = 24;   // Etwas größer für bessere Sichtbarkeit
            int vehicle_height = 36;
            
            // SCHWARZES Fahrzeug (realistisch)
            DrawRectangle(x - vehicle_width/2, y - vehicle_height/2, 
                         vehicle_width, vehicle_height, BLACK);
            
            // HOCHKONTRASTREICHE Markierungspunkte auf dem schwarzen Fahrzeug
            Color front_color = ORANGE;  // Orange vorne (einheitlich)
            Color rear_color = BLACK;    // Default
            
            // Optimierte Farben für weiße Projektionsfläche
            if (vehicle.rear_color == "Blau") rear_color = {0, 100, 255, 255};      // Helles Blau
            else if (vehicle.rear_color == "Grün") rear_color = {0, 255, 0, 255};   // Reines Grün
            else if (vehicle.rear_color == "Gelb") rear_color = {255, 255, 0, 255}; // Reines Gelb
            else if (vehicle.rear_color == "Lila") rear_color = {255, 0, 255, 255}; // Reines Magenta
            else if (vehicle.rear_color == "Rot") rear_color = {255, 0, 0, 255};    // Reines Rot
            
            // VORDERER Markierungspunkt (ORANGE)
            DrawCircle(x, y - vehicle_height/3, 6, front_color);
            
            // HINTERER Markierungspunkt (individuelle Farbe)
            DrawCircle(x, y + vehicle_height/3, 6, rear_color);
            
            // Fahrzeug-ID als Text (weiß auf schwarz für Kontrast)
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
    
    // Koordinatensystem-Legende (innerhalb des nutzbaren Bereichs)
    int legend_x = GetScreenWidth() - 270;
    int legend_y = 80;  // Unterhalb der Eckpunkte
    DrawText("FAHRZEUG-POSITIONEN", legend_x, legend_y, 16, BLACK);
    DrawText("Quadrat = Fahrzeug", legend_x, legend_y + 20, 12, DARKGRAY);
    DrawText("Linie = Fahrtrichtung", legend_x, legend_y + 35, 12, DARKGRAY);
    DrawText("Zahl = Fahrzeug-ID", legend_x, legend_y + 50, 12, DARKGRAY);
    DrawText("Unten = Vorwärts", legend_x, legend_y + 70, 10, DARKGRAY);
    DrawText("Rechts = Rechts", legend_x, legend_y + 85, 10, DARKGRAY);
    
    // Kamera-Bereich Information
    DrawText("ROTE ECKPUNKTE = Kamera-Erkennungsbereich", legend_x, legend_y + 105, 10, RED);
    DrawText("ESC = Beenden | F11 = Vollbild", legend_x, legend_y + 120, 10, DARKGRAY);
}

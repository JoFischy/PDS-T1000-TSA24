#include "raylib.h"
#include <iostream>

int main() {
    // Fenster-Konfiguration für Beamer-Projektion
    const int WINDOW_WIDTH = 1200;
    const int WINDOW_HEIGHT = 900;
    const int CORNER_SIZE = 20;  // Größe der roten Eckpunkte

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "BEAMER PROJEKTION - 4 Rote Eckpunkte");
    SetTargetFPS(60);

    // Fenster verschiebbar machen
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    std::cout << "=== BEAMER-PROJEKTION GESTARTET ===" << std::endl;
    std::cout << "4 ROTE ECKPUNKTE für Kamera-Orientierung" << std::endl;
    std::cout << "WEIßER HINTERGRUND für optimale Farberkennung" << std::endl;
    std::cout << "ESC = Beenden | F11 = Vollbild" << std::endl;
    std::cout << "===================================" << std::endl;

    while (!WindowShouldClose()) {
        // Eingabe-Verarbeitung
        if (IsKeyPressed(KEY_ESCAPE)) {
            break;
        }

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        // Aktualisierte Fenster-Dimensionen holen (bei Größenänderung)
        int current_width = GetScreenWidth();
        int current_height = GetScreenHeight();

        BeginDrawing();

        // WEIßER HINTERGRUND (wichtig für Farberkennung)
        ClearBackground(WHITE);

        // 4 ROTE ECKPUNKTE zeichnen
        // Oben Links
        DrawCircle(CORNER_SIZE, CORNER_SIZE, CORNER_SIZE, RED);

        // Oben Rechts  
        DrawCircle(current_width - CORNER_SIZE, CORNER_SIZE, CORNER_SIZE, RED);

        // Unten Links
        DrawCircle(CORNER_SIZE, current_height - CORNER_SIZE, CORNER_SIZE, RED);

        // Unten Rechts
        DrawCircle(current_width - CORNER_SIZE, current_height - CORNER_SIZE, CORNER_SIZE, RED);

        // Info-Text (klein und unauffällig)
        DrawText("BEAMER PROJEKTION", 10, 60, 20, LIGHTGRAY);
        DrawText("ESC=Exit F11=Fullscreen", 10, 85, 16, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();
    std::cout << "Beamer-Projektion beendet." << std::endl;

    return 0;
}
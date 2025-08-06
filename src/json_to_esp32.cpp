/**
 * @file json_to_esp32.cpp
 * @brief Liest Kamerakoordinaten aus coordinates.json und sendet sie an ESP32
 * 
 * Dieses Programm überwacht kontinuierlich die coordinates.json Datei
 * und sendet alle Heck-Koordinaten an das ESP32. Fehlende Hecks werden
 * als Fehlercodes gesendet.
 */

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include "../include/uart_communication.h"

// JSON-Parser (einfache Implementation ohne externe Bibliothek)
class SimpleJSONParser {
public:
    struct CoordinateData {
        std::string color;
        float x = -1;
        float y = -1;
        bool hasX = false;
        bool hasY = false;
        float area = 0;
    };
    
    static std::vector<CoordinateData> parseCoordinatesFromFile(const std::string& filename) {
        std::vector<CoordinateData> coordinates;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Konnte " << filename << " nicht öffnen" << std::endl;
            return coordinates;
        }
        
        std::string line;
        CoordinateData currentData;
        bool inObject = false;
        
        while (std::getline(file, line)) {
            // Entferne Whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            if (line.find("\"color\":") != std::string::npos) {
                // Extrahiere Farbe: "color": "Heck1",
                size_t start = line.find("\"", line.find(":") + 1) + 1;
                size_t end = line.find("\"", start);
                if (start != std::string::npos && end != std::string::npos) {
                    currentData.color = line.substr(start, end - start);
                    inObject = true;
                }
            }
            
            if (inObject && line.find("\"x\":") != std::string::npos) {
                // Extrahiere X-Koordinate: "x": 39.0,
                size_t colonPos = line.find(":");
                if (colonPos != std::string::npos) {
                    std::string valueStr = line.substr(colonPos + 1);
                    valueStr.erase(0, valueStr.find_first_not_of(" \t"));
                    valueStr.erase(valueStr.find_last_not_of(" \t,") + 1);
                    try {
                        currentData.x = std::stof(valueStr);
                        currentData.hasX = true;
                    } catch (...) {
                        std::cerr << "Fehler beim Parsen der X-Koordinate: " << valueStr << std::endl;
                    }
                }
            }
            
            if (inObject && line.find("\"y\":") != std::string::npos) {
                // Extrahiere Y-Koordinate: "y": 261.0
                size_t colonPos = line.find(":");
                if (colonPos != std::string::npos) {
                    std::string valueStr = line.substr(colonPos + 1);
                    valueStr.erase(0, valueStr.find_first_not_of(" \t"));
                    valueStr.erase(valueStr.find_last_not_of(" \t,") + 1);
                    try {
                        currentData.y = std::stof(valueStr);
                        currentData.hasY = true;
                    } catch (...) {
                        std::cerr << "Fehler beim Parsen der Y-Koordinate: " << valueStr << std::endl;
                    }
                }
            }
            
            if (inObject && line.find("\"area\":") != std::string::npos) {
                // Extrahiere Fläche: "area": 92.5
                size_t colonPos = line.find(":");
                if (colonPos != std::string::npos) {
                    std::string valueStr = line.substr(colonPos + 1);
                    valueStr.erase(0, valueStr.find_first_not_of(" \t"));
                    valueStr.erase(valueStr.find_last_not_of(" \t,") + 1);
                    try {
                        currentData.area = std::stof(valueStr);
                    } catch (...) {
                        // Area ist optional
                    }
                }
            }
            
            // Objekt Ende erkannt
            if (inObject && (line.find("}") != std::string::npos)) {
                if (!currentData.color.empty()) {
                    coordinates.push_back(currentData);
                }
                // Reset für nächstes Objekt
                currentData = CoordinateData();
                inObject = false;
            }
        }
        
        return coordinates;
    }
};

class HeckCoordinateManager {
private:
    UARTCommunication uart;
    std::string jsonFilename;
    std::map<std::string, std::chrono::steady_clock::time_point> lastUpdateTimes;
    
public:
    HeckCoordinateManager(const std::string& port, const std::string& jsonFile) 
        : uart(port), jsonFilename(jsonFile) {}
    
    bool initialize() {
        std::cout << "Initialisiere Heck-Koordinaten-Manager..." << std::endl;
        
        if (!uart.initialize()) {
            std::cerr << "UART Initialisierung fehlgeschlagen!" << std::endl;
            return false;
        }
        
        std::cout << "Manager erfolgreich initialisiert." << std::endl;
        return true;
    }
    
    std::vector<HeckCoordinate> createHeckCoordinatesFromJSON() {
        std::vector<HeckCoordinate> heckCoords;
        
        // Alle erwarteten Hecks definieren
        std::vector<std::string> expectedHecks = {"Heck1", "Heck2", "Heck3", "Heck4"};
        
        // JSON parsen
        auto jsonData = SimpleJSONParser::parseCoordinatesFromFile(jsonFilename);
        
        std::cout << "\n--- JSON Analyse ---" << std::endl;
        std::cout << "Gelesene Objekte: " << jsonData.size() << std::endl;
        
        // Map für schnelle Suche erstellen
        std::map<std::string, SimpleJSONParser::CoordinateData> foundHecks;
        for (const auto& data : jsonData) {
            if (data.color.find("Heck") == 0) {
                foundHecks[data.color] = data;
                std::cout << "Gefunden: " << data.color;
                if (data.hasX && data.hasY) {
                    std::cout << " (X=" << data.x << ", Y=" << data.y << ")";
                } else {
                    std::cout << " (unvollständige Koordinaten)";
                }
                std::cout << std::endl;
            }
        }
        
        // Für jedes erwartete Heck prüfen
        for (const auto& heckId : expectedHecks) {
            HeckCoordinate heckCoord;
            heckCoord.heckId = heckId;
            
            auto it = foundHecks.find(heckId);
            if (it != foundHecks.end() && it->second.hasX && it->second.hasY) {
                // Heck gefunden mit vollständigen Koordinaten
                heckCoord.x = it->second.x;
                heckCoord.y = it->second.y;
                heckCoord.isValid = true;
            } else {
                // Heck nicht gefunden oder unvollständige Koordinaten
                heckCoord.x = -1;
                heckCoord.y = -1;
                heckCoord.isValid = false;
            }
            
            heckCoords.push_back(heckCoord);
        }
        
        return heckCoords;
    }
    
    void runContinuousMonitoring() {
        std::cout << "\n=== Starte kontinuierliche JSON-Überwachung ===" << std::endl;
        std::cout << "Überwacht Datei: " << jsonFilename << std::endl;
        std::cout << "Drücken Sie Ctrl+C zum Beenden\n" << std::endl;
        
        auto lastModified = std::filesystem::last_write_time(jsonFilename);
        
        while (true) {
            try {
                // Prüfe ob JSON-Datei geändert wurde
                auto currentModified = std::filesystem::last_write_time(jsonFilename);
                
                if (currentModified != lastModified) {
                    std::cout << "📂 JSON-Datei wurde aktualisiert..." << std::endl;
                    lastModified = currentModified;
                    
                    // Warte kurz für komplette Schreiboperationen
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    
                    // Lese und sende neue Koordinaten
                    auto heckCoords = createHeckCoordinatesFromJSON();
                    uart.sendAllHeckCoordinates(heckCoords);
                    
                    std::cout << "⏰ Warte auf nächste Änderung..." << std::endl;
                }
                
                // Kurze Pause vor nächster Überprüfung
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
            } catch (const std::filesystem::filesystem_error& ex) {
                std::cerr << "Dateisystem-Fehler: " << ex.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    void runSingleShot() {
        std::cout << "\n=== Einmaliger JSON-zu-ESP32 Transfer ===" << std::endl;
        
        auto heckCoords = createHeckCoordinatesFromJSON();
        uart.sendAllHeckCoordinates(heckCoords);
        
        std::cout << "Transfer abgeschlossen." << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "=== JSON zu ESP32 Koordinaten-Sender ===" << std::endl;
    
    // Kommandozeilenparameter prüfen
    std::string jsonFile = "coordinates.json";
    std::string comPort = "COM4";
    bool continuousMode = true;
    
    if (argc > 1) {
        std::string mode(argv[1]);
        if (mode == "--single" || mode == "-s") {
            continuousMode = false;
        } else if (mode == "--help" || mode == "-h") {
            std::cout << "Verwendung: json_to_esp32 [--single|-s] [--port|-p COM4] [--file|-f coordinates.json]" << std::endl;
            std::cout << "  --single, -s    : Einmaliger Transfer (Standard: kontinuierlich)" << std::endl;
            std::cout << "  --port, -p      : COM-Port (Standard: COM4)" << std::endl;
            std::cout << "  --file, -f      : JSON-Datei (Standard: coordinates.json)" << std::endl;
            return 0;
        }
    }
    
    // Weitere Parameter verarbeiten
    for (int i = 1; i < argc - 1; i++) {
        std::string arg(argv[i]);
        if (arg == "--port" || arg == "-p") {
            comPort = argv[i + 1];
        } else if (arg == "--file" || arg == "-f") {
            jsonFile = argv[i + 1];
        }
    }
    
    std::cout << "Konfiguration:" << std::endl;
    std::cout << "  JSON-Datei: " << jsonFile << std::endl;
    std::cout << "  COM-Port: " << comPort << std::endl;
    std::cout << "  Modus: " << (continuousMode ? "Kontinuierlich" : "Einmalig") << std::endl;
    
    // Prüfe ob JSON-Datei existiert
    if (!std::filesystem::exists(jsonFile)) {
        std::cerr << "Fehler: JSON-Datei '" << jsonFile << "' nicht gefunden!" << std::endl;
        std::cerr << "Stellen Sie sicher, dass die Kameraerkennung läuft und Daten schreibt." << std::endl;
        return 1;
    }
    
    // Manager initialisieren
    HeckCoordinateManager manager(comPort, jsonFile);
    
    if (!manager.initialize()) {
        std::cerr << "Initialisierung fehlgeschlagen!" << std::endl;
        return 1;
    }
    
    // Entsprechenden Modus ausführen
    if (continuousMode) {
        manager.runContinuousMonitoring();
    } else {
        manager.runSingleShot();
    }
    
    return 0;
}

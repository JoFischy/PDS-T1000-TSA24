/**
 * @file json_monitor_debug.cpp
 * @brief Verbesserter JSON-Monitor mit Debug-Ausgaben
 */

#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <cstdlib>

class DebugJsonSender {
private:
    HANDLE hSerial;
    std::string port;
    std::string jsonFile;
    int messageCount;
    
public:
    DebugJsonSender(const std::string& comPort, const std::string& jsonFilePath) 
        : port(comPort), jsonFile(jsonFilePath), messageCount(0) {
        hSerial = INVALID_HANDLE_VALUE;
    }
    
    ~DebugJsonSender() {
        if (hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial);
        }
    }
    
    bool connect() {
        std::string fullPort = "\\\\.\\" + port;
        
        std::cout << "Versuche Verbindung zu " << fullPort << "..." << std::endl;
        
        hSerial = CreateFileA(fullPort.c_str(),
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
        
        if (hSerial == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            std::cout << "FEHLER beim Oeffnen von " << port << " (Error: " << error << ")" << std::endl;
            return false;
        }
        
        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        
        if (!GetCommState(hSerial, &dcbSerialParams)) {
            std::cout << "FEHLER beim Lesen der COM-Port Konfiguration" << std::endl;
            return false;
        }
        
        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        
        if (!SetCommState(hSerial, &dcbSerialParams)) {
            std::cout << "FEHLER beim Setzen der COM-Port Konfiguration" << std::endl;
            return false;
        }
        
        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        
        if (!SetCommTimeouts(hSerial, &timeouts)) {
            std::cout << "FEHLER beim Setzen der Timeouts" << std::endl;
            return false;
        }
        
        std::cout << "ERFOLGREICH verbunden mit " << port << " (115200 baud)" << std::endl;
        return true;
    }
    
    bool sendCommand(int direction) {
        if (hSerial == INVALID_HANDLE_VALUE) {
            std::cout << "FEHLER: Nicht verbunden!" << std::endl;
            return false;
        }
        
        int speed;
        switch (direction) {
            case 1: // Vorwärts
            case 2: // Rückwärts
                speed = 125;
                break;
            case 3: // Links  
            case 4: // Rechts
                speed = 160;
                break;
            default: // Stopp
                speed = 0;
                break;
        }
        
        // Format: direction,speed (ohne vehicle_id = an alle)
        std::string command = std::to_string(direction) + "," + std::to_string(speed) + "\n";
        
        std::cout << "Sende Befehl: '" << command.substr(0, command.length()-1) << "'" << std::endl;
        
        DWORD bytesWritten;
        bool success = WriteFile(hSerial, command.c_str(), command.length(), &bytesWritten, NULL);
        
        if (success && bytesWritten == command.length()) {
            messageCount++;
            std::cout << ">>> GESENDET #" << messageCount << " an ALLE Fahrzeuge: Direction=" << direction << ", Speed=" << speed << std::endl;
            std::cout << "    Bytes geschrieben: " << bytesWritten << "/" << command.length() << std::endl;
            return true;
        } else {
            DWORD error = GetLastError();
            std::cout << "FEHLER beim Senden (Error: " << error << ")" << std::endl;
            return false;
        }
    }
    
    void parseAndSend(const std::string& content) {
        std::cout << "Parse JSON: " << content << std::endl;
        
        // Einfacher Parser: suche nur "direction":
        size_t directionPos = content.find("\"direction\"");
        
        if (directionPos != std::string::npos) {
            int direction = 0;
            
            // Parse direction
            size_t colonPos = content.find(":", directionPos);
            if (colonPos != std::string::npos) {
                size_t numStart = content.find_first_of("0123456789", colonPos);
                if (numStart != std::string::npos) {
                    direction = atoi(content.substr(numStart, 1).c_str());
                }
            }
            
            std::cout << "Gefunden: Direction=" << direction << std::endl;
            
            // Validierung und Senden
            if (direction >= 0 && direction <= 5) {
                std::cout << ">>> JSON-BEFEHL: Direction=" << direction << " (an ALLE Fahrzeuge)" << std::endl;
                sendCommand(direction);
            } else {
                std::cout << "FEHLER: Ungueltige Direction: " << direction << std::endl;
            }
        } else {
            std::cout << "FEHLER: Kein 'direction' Feld gefunden" << std::endl;
        }
    }
    
    void monitorFile() {
        std::cout << "\n===============================================" << std::endl;
        std::cout << "JSON-MONITOR GESTARTET" << std::endl;
        std::cout << "===============================================" << std::endl;
        std::cout << "Datei: " << jsonFile << std::endl;
        std::cout << "Format: {\"direction\": 0-5}" << std::endl;
        std::cout << "Aktion: Wird an ALLE 4 Fahrzeuge gesendet!" << std::endl;
        std::cout << "Directions:" << std::endl;
        std::cout << "  1=Vor(125), 2=Zurueck(125)" << std::endl;
        std::cout << "  3=Links(160), 4=Rechts(160), 5=Stopp(0)" << std::endl;
        std::cout << "===============================================" << std::endl;
        
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        FILETIME lastWriteTime = {0};
        
        int loopCount = 0;
        while (true) {
            loopCount++;
            if (loopCount % 100 == 0) { // Alle 5 Sekunden
                std::cout << "Monitor aktiv... (Loop " << loopCount << ")" << std::endl;
            }
            
            if (GetFileAttributesExA(jsonFile.c_str(), GetFileExInfoStandard, &fileInfo)) {
                if (CompareFileTime(&fileInfo.ftLastWriteTime, &lastWriteTime) > 0) {
                    lastWriteTime = fileInfo.ftLastWriteTime;
                    
                    std::cout << "\n>>> DATEI GEAENDERT! <<<" << std::endl;
                    
                    std::ifstream file(jsonFile);
                    if (file.is_open()) {
                        std::string content((std::istreambuf_iterator<char>(file)),
                                           std::istreambuf_iterator<char>());
                        file.close();
                        
                        std::cout << "Datei-Inhalt: " << content << std::endl;
                        parseAndSend(content);
                    } else {
                        std::cout << "FEHLER: Kann Datei nicht oeffnen" << std::endl;
                    }
                    
                    std::cout << ">>> VERARBEITUNG ABGESCHLOSSEN <<<\n" << std::endl;
                }
            } else {
                if (loopCount == 1) {
                    std::cout << "WARNUNG: Datei nicht gefunden: " << jsonFile << std::endl;
                }
            }
            Sleep(50);
        }
    }
};

int main(int argc, char* argv[]) {
    std::string comPort = "COM3";
    std::string jsonFile = "test_commands.json";
    
    if (argc > 1) comPort = argv[1];
    if (argc > 2) jsonFile = argv[2];
    
    std::cout << "===============================================" << std::endl;
    std::cout << "ESP32 JSON DEBUG MONITOR" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << "COM-Port: " << comPort << std::endl;
    std::cout << "JSON-Datei: " << jsonFile << std::endl;
    std::cout << "===============================================" << std::endl;
    
    DebugJsonSender sender(comPort, jsonFile);
    
    if (!sender.connect()) {
        std::cout << "\nFEHLER: Verbindung fehlgeschlagen!" << std::endl;
        std::cout << "Verwendung: " << argv[0] << " [COM-Port] [JSON-Datei]" << std::endl;
        std::cout << "\nDruecken Sie eine Taste..." << std::endl;
        std::cin.get();
        return 1;
    }
    
    sender.monitorFile();
    return 0;
}

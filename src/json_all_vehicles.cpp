/**
 * @file json_all_vehicles.cpp
 * @brief JSON-basierter UART Sender für ALLE Fahrzeuge
 */

#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <cstdlib>

// JSON Command für alle Fahrzeuge
struct AllVehiclesCommand {
    int direction;
    bool valid;
};

class AllVehiclesJsonSender {
private:
    HANDLE hSerial;
    std::string port;
    std::string jsonFile;
    
public:
    AllVehiclesJsonSender(const std::string& comPort, const std::string& jsonFilePath) 
        : port(comPort), jsonFile(jsonFilePath) {
        hSerial = INVALID_HANDLE_VALUE;
    }
    
    ~AllVehiclesJsonSender() {
        if (hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial);
        }
    }
    
    bool connect() {
        std::string fullPort = "\\\\.\\" + port;
        
        hSerial = CreateFileA(fullPort.c_str(),
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
        
        if (hSerial == INVALID_HANDLE_VALUE) {
            std::cout << "Fehler beim Oeffnen von " << port << std::endl;
            return false;
        }
        
        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        
        if (!GetCommState(hSerial, &dcbSerialParams)) {
            std::cout << "Fehler beim Lesen der COM-Port Konfiguration" << std::endl;
            return false;
        }
        
        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        
        if (!SetCommState(hSerial, &dcbSerialParams)) {
            std::cout << "Fehler beim Setzen der COM-Port Konfiguration" << std::endl;
            return false;
        }
        
        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        
        if (!SetCommTimeouts(hSerial, &timeouts)) {
            std::cout << "Fehler beim Setzen der Timeouts" << std::endl;
            return false;
        }
        
        std::cout << "Verbunden mit " << port << " (115200 baud)" << std::endl;
        return true;
    }
    
    bool sendCommand(int direction) {
        if (hSerial == INVALID_HANDLE_VALUE) {
            std::cout << "Nicht verbunden!" << std::endl;
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
        
        DWORD bytesWritten;
        bool success = WriteFile(hSerial, command.c_str(), command.length(), &bytesWritten, NULL);
        
        if (success && bytesWritten == command.length()) {
            std::cout << "Gesendet an ALLE Fahrzeuge: Direction=" << direction << ", Speed=" << speed << std::endl;
            return true;
        } else {
            std::cout << "Fehler beim Senden" << std::endl;
            return false;
        }
    }
    
    void parseAndSend(const std::string& content) {
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
            
            // Validierung und Senden
            if (direction >= 0 && direction <= 5) {
                std::cout << "JSON gelesen: Direction=" << direction << " (an ALLE Fahrzeuge)" << std::endl;
                sendCommand(direction);
            }
        }
    }
    
    void monitorFile() {
        std::cout << "Ueberwache JSON-Datei: " << jsonFile << std::endl;
        std::cout << "Format: {\"direction\": 0-5} - Wird an ALLE 4 Fahrzeuge gesendet!" << std::endl;
        std::cout << "Directions: 1=Vor(125), 2=Zurueck(125), 3=Links(160), 4=Rechts(160), 5=Stopp(0)" << std::endl;
        
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        FILETIME lastWriteTime = {0};
        
        while (true) {
            if (GetFileAttributesExA(jsonFile.c_str(), GetFileExInfoStandard, &fileInfo)) {
                if (CompareFileTime(&fileInfo.ftLastWriteTime, &lastWriteTime) > 0) {
                    lastWriteTime = fileInfo.ftLastWriteTime;
                    
                    std::ifstream file(jsonFile);
                    if (file.is_open()) {
                        std::string content((std::istreambuf_iterator<char>(file)),
                                           std::istreambuf_iterator<char>());
                        file.close();
                        parseAndSend(content);
                    }
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
    
    std::cout << "ESP32 ALL VEHICLES Direction/Speed Sender (JSON-Monitor)" << std::endl;
    std::cout << "COM-Port: " << comPort << std::endl;
    std::cout << "JSON-Datei: " << jsonFile << std::endl;
    
    AllVehiclesJsonSender sender(comPort, jsonFile);
    
    if (!sender.connect()) {
        std::cout << "Verwendung: " << argv[0] << " [COM-Port] [JSON-Datei]" << std::endl;
        return 1;
    }
    
    sender.monitorFile();
    return 0;
}

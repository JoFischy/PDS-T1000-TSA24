/**
 * @file json_sender_simple.cpp
 * @brief Einfacher JSON-basierter UART Sender (C++-kompatibel)
 */

#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <cstdlib>

class SimpleJsonSender {
private:
    HANDLE hSerial;
    std::string port;
    std::string jsonFile;
    
public:
    SimpleJsonSender(const std::string& comPort, const std::string& jsonFilePath) 
        : port(comPort), jsonFile(jsonFilePath) {
        hSerial = INVALID_HANDLE_VALUE;
    }
    
    ~SimpleJsonSender() {
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
    
    bool sendCommand(int direction, int vehicle_id) {
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
        
        std::string command = std::to_string(direction) + "," + std::to_string(speed) + "," + std::to_string(vehicle_id) + "\n";
        
        DWORD bytesWritten;
        bool success = WriteFile(hSerial, command.c_str(), command.length(), &bytesWritten, NULL);
        
        if (success && bytesWritten == command.length()) {
            std::cout << "Gesendet (Fzg " << vehicle_id << "): Direction=" << direction << ", Speed=" << speed << std::endl;
            return true;
        } else {
            std::cout << "Fehler beim Senden" << std::endl;
            return false;
        }
    }
    
    void parseAndSend(const std::string& content) {
        // Einfacher Parser: suche "vehicle_id": und "direction":
        size_t vehiclePos = content.find("\"vehicle_id\"");
        size_t directionPos = content.find("\"direction\"");
        
        if (vehiclePos != std::string::npos && directionPos != std::string::npos) {
            int vehicle_id = 0;
            int direction = 0;
            
            // Parse vehicle_id
            size_t colonPos = content.find(":", vehiclePos);
            if (colonPos != std::string::npos) {
                size_t numStart = content.find_first_of("0123456789", colonPos);
                if (numStart != std::string::npos) {
                    vehicle_id = atoi(content.substr(numStart, 1).c_str());
                }
            }
            
            // Parse direction
            colonPos = content.find(":", directionPos);
            if (colonPos != std::string::npos) {
                size_t numStart = content.find_first_of("0123456789", colonPos);
                if (numStart != std::string::npos) {
                    direction = atoi(content.substr(numStart, 1).c_str());
                }
            }
            
            // Validierung und Senden
            if (vehicle_id >= 1 && vehicle_id <= 4 && direction >= 0 && direction <= 5) {
                std::cout << "JSON gelesen: Fahrzeug=" << vehicle_id << ", Direction=" << direction << std::endl;
                sendCommand(direction, vehicle_id);
            }
        }
    }
    
    void monitorFile() {
        std::cout << "Ueberwache JSON-Datei: " << jsonFile << std::endl;
        std::cout << "Format: {\"vehicle_id\": 1-4, \"direction\": 0-5}" << std::endl;
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
    std::string jsonFile = "coordinates.json";
    
    if (argc > 1) comPort = argv[1];
    if (argc > 2) jsonFile = argv[2];
    
    std::cout << "ESP32 Direction/Speed Sender (JSON-Monitor)" << std::endl;
    std::cout << "COM-Port: " << comPort << std::endl;
    std::cout << "JSON-Datei: " << jsonFile << std::endl;
    
    SimpleJsonSender sender(comPort, jsonFile);
    
    if (!sender.connect()) {
        std::cout << "Verwendung: " << argv[0] << " [COM-Port] [JSON-Datei]" << std::endl;
        return 1;
    }
    
    sender.monitorFile();
    return 0;
}

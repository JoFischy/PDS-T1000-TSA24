/**
 * @file direction_speed_sender.cpp
 * @brief Windows UART Sender für Direction/Speed-Befehle an ESP32
 * 
 * Kompilierung: g++ -o direction_speed_sender.exe direction_speed_sender.cpp
 * Verwendung: direction_speed_sender.exe [COM-Port]
 */

#include <iostream>
#include <string>
#include <windows.h>
#include <chrono>

class DirectionSpeedSender {
private:
    HANDLE hSerial;
    std::string port;
    int currentVehicle; // 1-4 = spezifisches Fahrzeug
    std::chrono::steady_clock::time_point lastSendTime;
    const int MIN_SEND_INTERVAL_MS = 100; // Mindestabstand zwischen Nachrichten
    
public:
    DirectionSpeedSender(const std::string& comPort) : port(comPort), currentVehicle(1) {
        hSerial = INVALID_HANDLE_VALUE;
        lastSendTime = std::chrono::steady_clock::now();
    }
    
    ~DirectionSpeedSender() {
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
            std::cerr << "❌ Fehler beim Öffnen von " << port << std::endl;
            return false;
        }
        
        // UART Konfiguration: 115200 baud, 8N1
        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        
        if (!GetCommState(hSerial, &dcbSerialParams)) {
            std::cerr << "❌ Fehler beim Lesen der COM-Port Konfiguration" << std::endl;
            return false;
        }
        
        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        
        if (!SetCommState(hSerial, &dcbSerialParams)) {
            std::cerr << "❌ Fehler beim Setzen der COM-Port Konfiguration" << std::endl;
            return false;
        }
        
        // Timeouts setzen
        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        
        if (!SetCommTimeouts(hSerial, &timeouts)) {
            std::cerr << "❌ Fehler beim Setzen der Timeouts" << std::endl;
            return false;
        }
        
        std::cout << "✅ Verbunden mit " << port << " (115200 baud)" << std::endl;
        return true;
    }
    
    bool sendCommand(int direction, int speed, int vehicle = -1) {
        if (hSerial == INVALID_HANDLE_VALUE) {
            std::cerr << "❌ Nicht verbunden!" << std::endl;
            return false;
        }
        
        // Rate-Limiting: Mindestabstand zwischen Nachrichten
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastSend = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSendTime).count();
        
        if (timeSinceLastSend < MIN_SEND_INTERVAL_MS) {
            Sleep(MIN_SEND_INTERVAL_MS - timeSinceLastSend);
        }
        
        // Verwende übergebenes vehicle oder currentVehicle
        int targetVehicle = (vehicle != -1) ? vehicle : currentVehicle;
        
        // Format: direction,speed,vehicle (vehicle immer erforderlich)
        std::string command = std::to_string(direction) + "," + std::to_string(speed) + "," + std::to_string(targetVehicle) + "\n";
        
        DWORD bytesWritten;
        bool success = WriteFile(hSerial, command.c_str(), command.length(), &bytesWritten, NULL);
        
        if (success && bytesWritten == command.length()) {
            std::cout << "📤 Gesendet (Fzg " << targetVehicle << "): " << command.substr(0, command.length()-1) << std::endl;
            lastSendTime = std::chrono::steady_clock::now();
            return true;
        } else {
            std::cerr << "❌ Fehler beim Senden" << std::endl;
            return false;
        }
    }
    
    void switchVehicle() {
        currentVehicle = (currentVehicle % 4) + 1; // 1-4, dann wieder 1
        std::cout << "🚗 Aktives Fahrzeug: " << currentVehicle << std::endl;
    }
    
    int getCurrentVehicle() const {
        return currentVehicle;
    }
    
    void showHelp() {
        std::cout << "\n🎮 ESP32 Direction/Speed Controller" << std::endl;
        std::cout << "====================================" << std::endl;
        std::cout << "📋 Directions:" << std::endl;
        std::cout << "  1 = Vorwärts" << std::endl;
        std::cout << "  2 = Rückwärts" << std::endl;
        std::cout << "  3 = Links" << std::endl;
        std::cout << "  4 = Rechts" << std::endl;
        std::cout << "  5 = Stopp" << std::endl;
        std::cout << "\n⚡ Speed: 120-255 (oder 0 bei Stopp)" << std::endl;
        std::cout << "\n� Fahrzeug-Auswahl:" << std::endl;
        std::cout << "  v = Fahrzeug wechseln (Alle -> 1 -> 2 -> 3 -> 4 -> Alle)" << std::endl;
        if (currentVehicle == 0) {
            std::cout << "  🎯 Aktuell: ALLE Fahrzeuge" << std::endl;
        } else {
            std::cout << "  🎯 Aktuell: Fahrzeug " << currentVehicle << std::endl;
        }
        std::cout << "\n�🔤 Schnell-Befehle:" << std::endl;
        std::cout << "  w = Vorwärts (200)" << std::endl;
        std::cout << "  s = Rückwärts (200)" << std::endl;
        std::cout << "  a = Links (180)" << std::endl;
        std::cout << "  d = Rechts (180)" << std::endl;
        std::cout << "  x = Stopp" << std::endl;
        std::cout << "  v = Fahrzeug wechseln" << std::endl;
        std::cout << "  h = Diese Hilfe" << std::endl;
        std::cout << "  q = Beenden" << std::endl;
        std::cout << "\n💬 Oder direkt eingeben:" << std::endl;
        std::cout << "  'direction,speed,vehicle' (z.B. '1,200,3') -> an Fahrzeug 3" << std::endl;
        std::cout << "===========================================" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::string comPort = "COM3"; // Default
    
    if (argc > 1) {
        comPort = argv[1];
    }
    
    std::cout << "🚀 ESP32 Direction/Speed Sender" << std::endl;
    std::cout << "Verwende COM-Port: " << comPort << std::endl;
    
    DirectionSpeedSender sender(comPort);
    
    if (!sender.connect()) {
        std::cout << "\nVerwendung: " << argv[0] << " [COM-Port]" << std::endl;
        std::cout << "Beispiel: " << argv[0] << " COM5" << std::endl;
        return 1;
    }
    
    sender.showHelp();
    
    std::cout << "\n🎯 Bereit für Eingaben (drücke 'h' für Hilfe):" << std::endl;
    
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        char cmd = input[0];
        
        switch (cmd) {
            case 'w': // Vorwärts
                sender.sendCommand(1, 200);
                break;
            case 's': // Rückwärts
                sender.sendCommand(2, 200);
                break;
            case 'a': // Links
                sender.sendCommand(3, 180);
                break;
            case 'd': // Rechts
                sender.sendCommand(4, 180);
                break;
            case 'x': // Stopp
                sender.sendCommand(5, 0);
                break;
            case 'v': // Fahrzeug wechseln
                sender.switchVehicle();
                break;
            case 'h': // Hilfe
                sender.showHelp();
                break;
            case 'q': // Beenden
                std::cout << "👋 Auf Wiedersehen!" << std::endl;
                return 0;
            default:
                // Versuche direction,speed,vehicle zu parsen
                size_t firstComma = input.find(',');
                if (firstComma != std::string::npos) {
                    size_t secondComma = input.find(',', firstComma + 1);
                    if (secondComma != std::string::npos) {
                        // Format: direction,speed,vehicle (erforderlich)
                        try {
                            int direction = std::stoi(input.substr(0, firstComma));
                            int speed = std::stoi(input.substr(firstComma + 1, secondComma - firstComma - 1));
                            int vehicle = std::stoi(input.substr(secondComma + 1));
                            
                            if (direction >= 1 && direction <= 5 && vehicle >= 1 && vehicle <= 4) {
                                sender.sendCommand(direction, speed, vehicle);
                            } else {
                                std::cout << "❌ Direction: 1-5, Vehicle: 1-4" << std::endl;
                            }
                        } catch (const std::exception& e) {
                            std::cout << "❌ Ungültiges Format. Verwende 'direction,speed,vehicle' oder 'h' für Hilfe" << std::endl;
                        }
                    } else {
                        std::cout << "❌ Vehicle-ID fehlt. Format: 'direction,speed,vehicle' (z.B. '1,200,3')" << std::endl;
                    }
                } else {
                    std::cout << "❌ Unbekannter Befehl. Drücke 'h' für Hilfe" << std::endl;
                }
                break;
        }
    }
    
    return 0;
}

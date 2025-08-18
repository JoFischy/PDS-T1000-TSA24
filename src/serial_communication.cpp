#include "serial_communication.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

// JSON parsing (simple manual parsing)
#include <regex>

SerialCommunication::SerialCommunication() : hSerial(INVALID_HANDLE_VALUE), isConnected(false) {}

SerialCommunication::~SerialCommunication() {
    disconnect();
}

bool SerialCommunication::connect(const std::string& port, int baudRate) {
    // Trennen falls bereits verbunden
    disconnect();
    
    portName = port;
    std::string fullPortName = "\\\\.\\" + port;
    
    // COM-Port öffnen
    hSerial = CreateFileA(
        fullPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "❌ Fehler beim Öffnen von " << port << std::endl;
        return false;
    }
    
    // COM-Port konfigurieren
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "❌ Fehler beim Lesen der COM-Port Einstellungen" << std::endl;
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        return false;
    }
    
    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "❌ Fehler beim Setzen der COM-Port Einstellungen" << std::endl;
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
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
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        return false;
    }
    
    isConnected = true;
    std::cout << "✅ Verbunden mit ESP-Board auf " << port << " (" << baudRate << " baud)" << std::endl;
    
    // Kurze Pause um ESP zeit zu geben
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    return true;
}

void SerialCommunication::disconnect() {
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        isConnected = false;
        std::cout << "🔌 ESP-Board Verbindung getrennt" << std::endl;
    }
}

bool SerialCommunication::sendData(const std::string& data) {
    if (!isConnected || hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "❌ Nicht mit ESP-Board verbunden" << std::endl;
        return false;
    }
    
    DWORD bytesWritten;
    if (!WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL)) {
        std::cerr << "❌ Fehler beim Senden der Daten" << std::endl;
        return false;
    }
    
    std::cout << "📤 Gesendet: " << data;
    return bytesWritten == data.length();
}

std::vector<std::string> SerialCommunication::getAvailablePorts() {
    std::vector<std::string> ports;
    
    // Prüfe COM1 bis COM20
    for (int i = 1; i <= 20; i++) {
        std::string portName = "COM" + std::to_string(i);
        std::string fullPortName = "\\\\.\\" + portName;
        
        HANDLE hTest = CreateFileA(
            fullPortName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        
        if (hTest != INVALID_HANDLE_VALUE) {
            ports.push_back(portName);
            CloseHandle(hTest);
        }
    }
    
    return ports;
}

bool SerialCommunication::sendVehicleCommands() {
    if (!isConnected) {
        std::cerr << "❌ Nicht mit ESP-Board verbunden" << std::endl;
        return false;
    }
    
    // vehicle_commands.json einlesen
    std::ifstream jsonFile("vehicle_commands.json");
    if (!jsonFile.is_open()) {
        std::cerr << "❌ Kann vehicle_commands.json nicht öffnen" << std::endl;
        return false;
    }
    
    std::stringstream jsonBuffer;
    jsonBuffer << jsonFile.rdbuf();
    std::string jsonContent = jsonBuffer.str();
    jsonFile.close();
    
    // Einfache JSON-Parsing mit Regex (für das spezifische Format)
    std::regex vehicleRegex(R"("id":\s*(\d+),\s*"command":\s*(\d+))");
    std::sregex_iterator regexIter(jsonContent.begin(), jsonContent.end(), vehicleRegex);
    std::sregex_iterator regexEnd;
    
    bool success = true;
    int commandCount = 0;
    
    std::cout << "📋 Sende Fahrzeugbefehle an ESP-Board..." << std::endl;
    
    for (; regexIter != regexEnd; ++regexIter) {
        const std::smatch& match = *regexIter;
        int vehicleId = std::stoi(match[1].str());
        int direction = std::stoi(match[2].str());
        
        // Speed basierend auf Direction bestimmen
        int speed = 0;
        if (direction == 5 || direction == 0) {
            speed = 0; // Stopp
        } else if (direction == 1 || direction == 2) {
            speed = 130; // Vorwärts/Rückwärts Geschwindigkeit
        } else if (direction == 3 || direction == 4) {
            speed = 160; // Links/Rechts Geschwindigkeit
        } else {
            speed = 130; // Standard-Geschwindigkeit für andere Richtungen
        }
        
        std::cout << "🚗 Fahrzeug " << vehicleId << ": Direction=" << direction << ", Speed=" << speed << std::endl;
        
        // Befehl im ESP-Format senden: "direction,speed"
        if (!sendCommand(direction, speed)) {
            success = false;
        }
        
        commandCount++;
        
        // Minimale Pause zwischen Befehlen (nur für Übertragungssicherheit)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    if (commandCount == 0) {
        std::cout << "⚠️ Keine Fahrzeugbefehle in JSON gefunden" << std::endl;
        return false;
    }
    
    std::cout << "✅ " << commandCount << " Fahrzeugbefehle gesendet" << std::endl;
    return success;
}

bool SerialCommunication::sendCommand(int direction, int speed) {
    if (!isConnected) {
        return false;
    }
    
    // ESP erwartet Format: "direction,speed\n"
    std::string command = std::to_string(direction) + "," + std::to_string(speed) + "\n";
    
    std::cout << "📡 Sende Befehl: Direction=" << direction << ", Speed=" << speed << std::endl;
    
    return sendData(command);
}

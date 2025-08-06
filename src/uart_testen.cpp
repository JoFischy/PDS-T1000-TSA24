/**
 * @file uart_testen.cpp
 * @brief Windows Serial Communication zum ESP32
 * 
 * Dieses Programm sendet Koordinatendaten über die serielle Schnittstelle
 * an das ESP32 für ESP-NOW Kommunikation zwischen Fahrzeugen.
 */

#include <iostream>
#include <Windows.h>
#include <string>
#include <thread>
#include <chrono>
#include <random>

class SerialComm {
private:
    HANDLE hSerial;
    std::string portName;
    
public:
    SerialComm(const std::string& port = "COM4") : portName(port), hSerial(INVALID_HANDLE_VALUE) {}
    
    ~SerialComm() {
        if (hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial);
        }
    }
    
    bool initialize() {
        // Serielle Schnittstelle öffnen
        hSerial = CreateFileA(portName.c_str(), 
                             GENERIC_WRITE | GENERIC_READ, 
                             0, 
                             0, 
                             OPEN_EXISTING, 
                             0, 
                             0);
        
        if (hSerial == INVALID_HANDLE_VALUE) {
            std::cerr << "Fehler beim Öffnen der seriellen Schnittstelle " << portName << std::endl;
            DWORD error = GetLastError();
            std::cerr << "Windows Fehlercode: " << error << std::endl;
            return false;
        }
        
        // Baudrate und Einstellungen konfigurieren
        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        
        if (!GetCommState(hSerial, &dcbSerialParams)) {
            std::cerr << "Fehler beim Abrufen der Comm State" << std::endl;
            return false;
        }
        
        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
        dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;
        
        if (!SetCommState(hSerial, &dcbSerialParams)) {
            std::cerr << "Fehler beim Setzen der Comm State" << std::endl;
            return false;
        }
        
        // Timeouts konfigurieren
        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        
        if (!SetCommTimeouts(hSerial, &timeouts)) {
            std::cerr << "Fehler beim Setzen der Timeouts" << std::endl;
            return false;
        }
        
        std::cout << "Serielle Schnittstelle " << portName << " erfolgreich initialisiert" << std::endl;
        return true;
    }
    
    bool sendCoordinates(float x, float y) {
        if (hSerial == INVALID_HANDLE_VALUE) {
            std::cerr << "Serielle Schnittstelle nicht initialisiert" << std::endl;
            return false;
        }
        
        // Datenformat: "X:12.34;Y:56.78;\n"
        std::string data = "X:" + std::to_string(x) + ";Y:" + std::to_string(y) + ";\n";
        
        DWORD bytesWritten;
        BOOL result = WriteFile(hSerial, data.c_str(), static_cast<DWORD>(data.size()), &bytesWritten, NULL);
        
        if (!result) {
            std::cerr << "Fehler beim Schreiben der Daten" << std::endl;
            return false;
        }
        
        std::cout << "Gesendet: " << data.substr(0, data.length()-1) << " (" << bytesWritten << " bytes)" << std::endl;
        return true;
    }
    
    void flushBuffers() {
        if (hSerial != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(hSerial);
        }
    }
};

class CoordinateGenerator {
private:
    std::mt19937 gen;
    std::uniform_real_distribution<float> x_dist;
    std::uniform_real_distribution<float> y_dist;
    
public:
    CoordinateGenerator() : gen(std::random_device{}()), x_dist(0.0f, 100.0f), y_dist(0.0f, 100.0f) {}
    
    std::pair<float, float> generateRandomCoordinates() {
        return {x_dist(gen), y_dist(gen)};
    }
};

int main() {
    std::cout << "=== ESP32 UART Koordinaten-Sender ===" << std::endl;
    std::cout << "Drücken Sie Ctrl+C zum Beenden" << std::endl << std::endl;
    
    // Serielle Verbindung initialisieren
    SerialComm serial("COM4"); // Anpassen je nach verwendetem COM-Port
    
    if (!serial.initialize()) {
        std::cerr << "Initialisierung fehlgeschlagen!" << std::endl;
        std::cout << "Bitte überprüfen Sie:" << std::endl;
        std::cout << "1. Ist das ESP32 verbunden?" << std::endl;
        std::cout << "2. Ist der COM-Port korrekt? (aktuell: COM4)" << std::endl;
        std::cout << "3. Wird der Port von einer anderen Anwendung verwendet?" << std::endl;
        return 1;
    }
    
    // Kurz warten bis ESP32 bereit ist
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    CoordinateGenerator generator;
    
    std::cout << "Beginne mit dem Senden von Koordinaten..." << std::endl;
    
    // Hauptschleife - Koordinaten senden
    int counter = 0;
    while (true) {
        counter++;
        
        // Beispiel-Koordinaten (können auch aus Datei oder Simulation kommen)
        auto [x, y] = generator.generateRandomCoordinates();
        
        // Oder feste Testkoordinaten:
        // float x = 10.0f + (counter % 10);
        // float y = 20.0f + (counter % 5);
        
        if (!serial.sendCoordinates(x, y)) {
            std::cerr << "Fehler beim Senden!" << std::endl;
            break;
        }
        
        serial.flushBuffers();
        
        // Warten vor nächster Übertragung (1 Sekunde)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Status ausgeben
        if (counter % 10 == 0) {
            std::cout << "--- " << counter << " Nachrichten gesendet ---" << std::endl;
        }
    }
    
    std::cout << "Programm beendet." << std::endl;
    return 0;
}

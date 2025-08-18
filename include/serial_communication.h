#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <windows.h>
#include <string>
#include <vector>

class SerialCommunication {
private:
    HANDLE hSerial;
    std::string portName;
    bool isConnected;
    
public:
    SerialCommunication();
    ~SerialCommunication();
    
    // Verbindung zur seriellen Schnittstelle herstellen
    bool connect(const std::string& port, int baudRate = 115200);
    
    // Verbindung trennen
    void disconnect();
    
    // Daten senden
    bool sendData(const std::string& data);
    
    // Status prüfen
    bool isConnectedToESP() const { return isConnected; }
    
    // Verfügbare COM-Ports auflisten
    static std::vector<std::string> getAvailablePorts();
    
    // Fahrzeugbefehle aus JSON senden
    bool sendVehicleCommands();
    
    // Einzelnen Befehl senden (direction, speed)
    bool sendCommand(int direction, int speed);
};

#endif

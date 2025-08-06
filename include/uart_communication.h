#pragma once
#include <string>
#include <vector>

struct HeckCoordinate {
    std::string heckId;      // "Heck1", "Heck2", etc.
    float x;
    float y;
    bool isValid;            // true wenn Koordinaten vorhanden, false für Fehlercode
};

class UARTCommunication {
private:
    void* hSerial;  // HANDLE as void* to avoid Windows.h include
    std::string portName;
    bool isConnected;
    
public:
    UARTCommunication(const std::string& port = "COM5");
    ~UARTCommunication();
    
    bool initialize();
    bool sendMessage(const std::string& message);  // Neue generische Funktion
    bool sendCoordinates(float x, float y);
    bool sendHeck2Coordinates(float x, float y);
    bool sendHeckCoordinates(const std::string& heckId, float x, float y);
    bool sendHeckError(const std::string& heckId);
    bool sendAllHeckCoordinates(const std::vector<HeckCoordinate>& hecks);
    void close();
    bool isInitialized() const { return isConnected; }
};

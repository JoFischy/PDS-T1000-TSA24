#pragma once
#include <string>

class UARTCommunication {
private:
    void* hSerial;  // HANDLE as void* to avoid Windows.h include
    std::string portName;
    bool isConnected;
    
public:
    UARTCommunication(const std::string& port = "COM5");
    ~UARTCommunication();
    
    bool initialize();
    bool sendCoordinates(float x, float y);
    bool sendHeck2Coordinates(float x, float y);
    void close();
    bool isInitialized() const { return isConnected; }
};

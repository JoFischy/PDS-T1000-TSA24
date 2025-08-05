#include "../include/uart_communication.h"
#include <iostream>
#include <Windows.h>

UARTCommunication::UARTCommunication(const std::string& port) 
    : portName(port), hSerial(INVALID_HANDLE_VALUE), isConnected(false) {
}

UARTCommunication::~UARTCommunication() {
    close();
}

bool UARTCommunication::initialize() {
    std::cout << "Initialisiere UART Verbindung auf " << portName << "..." << std::endl;
    
    // Serielle Schnittstelle öffnen
    HANDLE serial = CreateFileA(portName.c_str(), 
                         GENERIC_WRITE | GENERIC_READ, 
                         0, 
                         0, 
                         OPEN_EXISTING, 
                         0, 
                         0);
    
    if (serial == INVALID_HANDLE_VALUE) {
        std::cerr << "Fehler beim Öffnen der seriellen Schnittstelle " << portName << std::endl;
        DWORD error = GetLastError();
        std::cerr << "Windows Fehlercode: " << error << std::endl;
        return false;
    }
    
    hSerial = serial;
    
    // Baudrate und Einstellungen konfigurieren
    DCB dcbSerialParams = {};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    
    if (!GetCommState((HANDLE)hSerial, &dcbSerialParams)) {
        std::cerr << "Fehler beim Abrufen der Comm State" << std::endl;
        close();
        return false;
    }
    
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;
    
    if (!SetCommState((HANDLE)hSerial, &dcbSerialParams)) {
        std::cerr << "Fehler beim Setzen der Comm State" << std::endl;
        close();
        return false;
    }
    
    // Timeouts konfigurieren
    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    
    if (!SetCommTimeouts((HANDLE)hSerial, &timeouts)) {
        std::cerr << "Fehler beim Setzen der Timeouts" << std::endl;
        close();
        return false;
    }
    
    isConnected = true;
    std::cout << "UART Verbindung auf " << portName << " erfolgreich initialisiert (115200 baud)" << std::endl;
    return true;
}

bool UARTCommunication::sendCoordinates(float x, float y) {
    if (!isConnected || hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Datenformat: "X:12.34;Y:56.78;\n"
    std::string data = "X:" + std::to_string(x) + ";Y:" + std::to_string(y) + ";\n";
    
    DWORD bytesWritten;
    BOOL result = WriteFile((HANDLE)hSerial, data.c_str(), static_cast<DWORD>(data.size()), &bytesWritten, NULL);
    
    if (!result) {
        std::cerr << "Fehler beim Schreiben der UART Daten" << std::endl;
        return false;
    }
    
    // Buffer leeren um sicherzustellen dass Daten gesendet werden
    FlushFileBuffers((HANDLE)hSerial);
    
    std::cout << "UART gesendet: " << data.substr(0, data.length()-1) << " (" << bytesWritten << " bytes)" << std::endl;
    return true;
}

bool UARTCommunication::sendHeck2Coordinates(float x, float y) {
    if (!isConnected) {
        return false;
    }
    
    // Spezielle Markierung für Heck2 Koordinaten
    std::string data = "HECK2:X:" + std::to_string(x) + ";Y:" + std::to_string(y) + ";\n";
    
    DWORD bytesWritten;
    BOOL result = WriteFile((HANDLE)hSerial, data.c_str(), static_cast<DWORD>(data.size()), &bytesWritten, NULL);
    
    if (!result) {
        std::cerr << "Fehler beim Schreiben der Heck2 UART Daten" << std::endl;
        return false;
    }
    
    FlushFileBuffers((HANDLE)hSerial);
    
    std::cout << "UART → ESP32: " << data.substr(0, data.length()-1) << " (" << bytesWritten << " bytes)" << std::endl;
    return true;
}

void UARTCommunication::close() {
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle((HANDLE)hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
    isConnected = false;
}

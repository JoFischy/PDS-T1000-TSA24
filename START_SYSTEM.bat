@echo off
echo ====================================
echo PDS-T1000-TSA24 Fahrzeug-Erkennungssystem
echo ====================================
echo.
echo Starte System...
echo - Kamera 1 (DirectShow Backend)
echo - UART COM5 (115200 baud)  
echo - ESP32 ESP-NOW Bridge
echo.
echo Steuerung:
echo - ESC = Beenden
echo - F11 = Vollbild
echo.
echo ====================================
echo.

main_uart.exe

echo.
echo ====================================
echo System beendet.
echo ====================================
pause

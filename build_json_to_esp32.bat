@echo off
echo === JSON zu ESP32 Koordinaten-Sender Build ===
echo.

REM Prüfen ob g++ verfügbar ist
where g++ >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo FEHLER: g++ nicht gefunden!
    echo Bitte installieren Sie MinGW oder verwenden Sie Visual Studio Developer Command Prompt
    pause
    exit /b 1
)

echo Kompiliere json_to_esp32.cpp mit uart_communication.cpp...
g++ -std=c++17 -Wall -Wextra -O2 ^
    src/json_to_esp32.cpp ^
    src/uart_communication.cpp ^
    -I include ^
    -o json_to_esp32.exe ^
    -static-libgcc -static-libstdc++

if %ERRORLEVEL% equ 0 (
    echo.
    echo ✓ Build erfolgreich!
    echo   Programm: json_to_esp32.exe
    echo.
    echo Verwendung:
    echo   json_to_esp32.exe                    ^(kontinuierliche Überwachung^)
    echo   json_to_esp32.exe --single           ^(einmaliger Transfer^)
    echo   json_to_esp32.exe --port COM5        ^(anderer COM-Port^)
    echo   json_to_esp32.exe --help             ^(Hilfe^)
    echo.
    echo WICHTIG: 
    echo - ESP32 muss mit dem richtigen COM-Port verbunden sein
    echo - coordinates.json muss existieren ^(von der Kameraerkennung erstellt^)
) else (
    echo.
    echo ✗ Build fehlgeschlagen!
    echo Bitte prüfen Sie die Fehlermeldungen oben.
)

echo.
pause

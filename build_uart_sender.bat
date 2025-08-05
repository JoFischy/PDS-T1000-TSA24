@echo off
echo === ESP32 UART Sender Build ===
echo.

REM Prüfen ob g++ verfügbar ist
where g++ >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo FEHLER: g++ nicht gefunden!
    echo Bitte installieren Sie MinGW oder Visual Studio Build Tools
    echo.
    echo Download MinGW: https://www.mingw-w64.org/
    echo Oder verwenden Sie Visual Studio Developer Command Prompt
    pause
    exit /b 1
)

echo Kompiliere uart_sender.cpp...
g++ -std=c++17 -Wall -Wextra -O2 src/uart_sender.cpp -o uart_sender.exe -static-libgcc -static-libstdc++

if %ERRORLEVEL% equ 0 (
    echo.
    echo ✓ Build erfolgreich!
    echo   Programm: uart_sender.exe
    echo.
    echo Führen Sie uart_sender.exe aus um das Programm zu starten.
    echo WICHTIG: Stellen Sie sicher dass das ESP32 mit COM4 verbunden ist
    echo          oder passen Sie den COM-Port im Code an.
) else (
    echo.
    echo ✗ Build fehlgeschlagen!
    echo Bitte prüfen Sie die Fehlermeldungen oben.
)

echo.
pause

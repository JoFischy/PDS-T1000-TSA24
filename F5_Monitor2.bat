@echo off
echo ===============================================
echo F5 VOLLSTÄNDIGES KAMERA-TRACKING-SYSTEM
echo ===============================================
echo Building C++ + Python Integration...
echo y | call build.bat
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)
echo Build successful!
echo.
echo Starte Kamera-Tracking-System auf Monitor 2...
echo (Embedded Python + Raylib Integration)
.\main.exe --monitor2
